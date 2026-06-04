# 智能小车工程架构重构 — 面向 2024 电赛 H 题 (修订版)

## Context

当前工程已有基本功能（传感器读取、LCD 显示、偏航角恢复 P 控制器、Flash 存储），但存在以下问题：
- 硬件初始化分散在各任务中
- 按键处理耦合在传感器任务内
- LCD 无菜单系统
- 控制算法仅为偏航角恢复，不支持竞赛循迹
- 无蜂鸣器驱动（电赛要求顶点声光提示）

用户修正要点:
- **Sensor 优先级最高** — 传感器是闭环控制的数据源，状态解算依赖它
- **灰度 12 位数字量** — 每通道仅 0/1（高低电平），非模拟值
- **赛道几何纠正** — 标准田径跑道形，仅两段半圆弧是黑色线，直线段白色无迹可循
- **Sensor 提升到 100Hz** — 与控制循环同步

---

## 0. 赛道几何与竞赛任务

### 赛道布局 (220cm×120cm 白底)

```
          A(-50,40) ──── 100cm 直线(白色) ──── B(50,40)
             |                                    |
        LEFT ARC                            RIGHT ARC
     (x+50)²+y²=40²                    (x-50)²+y²=40²
       x ≤ -50                             x ≥ 50
       黑色弧线                            黑色弧线
             |                                    |
          D(-50,-40)──── 100cm 直线(白色) ──── C(50,-40)
```

- 两段 40cm 半径半圆弧，线宽 1.8cm，黑色
- 直线段 A-B (100cm) 和 C-D (100cm) 为白色背景，**无迹可循**
- 对角线 A-C 和 B-D 也无标记
- 右弧平移 100cm 后与左弧圆心重合，恰好拼成完整圆

### 四项任务

| 任务 | 路径 | 时限 | 分值 |
|------|------|------|------|
| (1) | A → B (任意路径) | ≤15s | 20 |
| (2) | A→B→右弧→C→D→左弧→A (1圈) | ≤30s | 20 |
| (3) | A→C→右弧→B→D→左弧→A (对角) | ≤40s | 30 |
| (4) | 按任务3路径自动行驶 4 圈 | 车速越快越好 | 30 |

**弧线端点:**
- 左弧 (x≤-50): A(-50,40) ↔ D(-50,-40)
- 右弧 (x≥50):  B(50,40) ↔ C(50,-40)

关键点:
- 不需要走直线，只要能到达对应点位即可
- 经过每个点必须声光提示
- 弧线段车身投影不能脱离黑线
- 禁止后退、遥控、摄像头

---

## 1. main.c 硬件初始化序列

所有硬件初始化移至 `main()`，在调度器启动前按序执行：

```
Step 1:  SYSCFG_DL_init()                              // SysConfig 全外设初始化
Step 2:  DL_GPIO_setPins(PB6)                           // W25Q128 CS=HIGH
Step 3:  WWDT 初始化 (~8s 超时, DL_WWDT_restart)         // 看门狗
Step 4:  NVIC_SetPriority (DMA/SPI→1, I2C/GPIO→2, TIMA→3)
         DL_UART_Main_disableInterrupt(UART0 RX)
Step 5:  BSP_delay_ms(1000)                              // 外设稳定
Step 6:  app_flash_init() → load config or run wizard     // [保留] 首次上电按键绑定
Step 7:  Motor_init()                                    // [移入] GPIO方向+PWM+编码器
Step 8:  ST7789_init(ST7789_HORIZONTAL)                   // [移入] LCD 硬件复位
Step 9:  ST7789_backLight(1) + Splash 屏幕               // 启动画面
Step 10: Buzzer_init(PB22)                               // [新增] 蜂鸣器, 高电平响
Step 11: 创建队列 (sensor_q, button_q, cmd_q)
Step 12: 创建任务 (Button→LCD→Control→Sensor 最后创建, 最高优先级)
Step 13: vTaskStartScheduler()
```

---

## 2. 新任务架构

### 任务总览

| # | 任务名 | 优先级 | 栈(words) | 周期 | 职责 |
|---|--------|--------|-----------|------|------|
| 1 | Sensor | idle+6 (最高) | 768 | 10ms (100Hz) | IMU+灰度+编码器+LED心跳+喂狗 |
| 2 | Control | idle+5 | 1024 | 10ms (100Hz) | 竞赛状态机+前馈+PID, 直接调 Motor_set |
| 3 | LCD | idle+4 | 1536 | 50ms (20Hz) | 多级菜单, 消费 button_queue |
| 4 | Button | idle+3 | 256 | 20ms (50Hz) | 按键扫描+重映射, 生产 button_queue |

**优先级理由:**
- **Sensor(6)**: 最高优先级，闭环控制数据源，100Hz 不能丢帧。先更新传感器 → Control 拿到最新数据
- **Control(5)**: 紧随 Sensor，100Hz 控制循环。每次 Sensor 运行完立即执行
- **LCD(4)**: UI 响应，20Hz 刷新率
- **Button(3)**: 人机输入，50Hz 扫描

**时序分析 (每 10ms 窗口):**
```
Sensor: ~3ms → Control: ~2ms → 空闲 ~5ms (LCD/Button 抢占)
每 50ms: LCD 渲染一帧 (~5ms via DMA)
每 20ms: Button 扫描 (~0.3ms)
```

### 任务间通信

```
TaskSensor(100Hz) ──xQueueOverwrite──→ sensor_queue (mailbox, 1 deep)
     │
     │ 直接写入 volatile:
     │   g_current_yaw  (float)
     │   g_sensor_data  (sensor_data_t)
     │
     ├──→ TaskControl (读 g_current_yaw + g_sensor_data, 直接调 Motor_set)
     │
     └──→ TaskLcd (消费 sensor_queue, 更新状态显示)

TaskButton(50Hz) ──xQueueSend──→ button_queue (8 deep)
     │
     └──→ TaskLcd (消费 button_queue, 驱动菜单导航)

TaskLcd ──xQueueSend──→ cmd_queue (4 deep)
     │
     └──→ TaskControl (接收竞赛启动/停止/急停指令)
```

### SPI1 共享协议

二值信号量 `g_spi1_mutex` (100ms 超时):
- 拿信号量 → 操作 SPI → 释放
- Flash 擦除忙等待(45ms)期间**释放信号量**让 LCD 使用，擦除完成后再拿回
- 拿不到信号量: LCD 跳过当前帧，Flash 返回 false (调用者重试)

---

## 3. Sensor 任务 (idle+6, 100Hz)

### 职责

1. 读取 JY61P 角度 (I2C0 DMA, 6 字节)
2. 读取 JY61P IMU 加速度+角速度 (I2C0 DMA, 12 字节)  
3. 读取 NCHD12 灰度 (I2C1 DMA, 2 字节 → 12 位位图)
4. 读取编码器距离 (volatile 读 g_enc1/g_enc2)
5. 更新原始传感器数据:
   - `g_current_yaw = JY61P_getTotalYaw()` (原始 IMU 累积偏航角)
   - `g_sensor_data` (完整原始传感器数据包)
6. 灰度质心计算: `line_position = centroid(gray_12bit)` → -5.5~+5.5
7. 编码器速度: `enc_speed = Δdist / 10ms`
8. **卡尔曼滤波**: 执行预测步骤，若在弧线上则执行测量更新
9. LED 心跳: 每 50 次循环 (500ms) 翻转 PB20
10. 喂狗: 每次循环 `DL_WWDT_restart(WWDT0)`
11. xQueueOverwrite(sensor_queue, &data)

### 数据结构

```c
typedef struct {
    bool     jy61p_ok, nchd12_ok;
    float    roll, pitch, yaw, total_yaw;    // 角度
    float    ax, ay, az;                      // 加速度 (m/s²)
    float    gx, gy, gz;                      // 角速度 (°/s)
    float    enc1_dist, enc2_dist;            // 编码器距离 (mm)
    float    enc_speed;                       // 瞬时速度 (mm/s)
    uint16_t grayscale;                       // 12 位灰度位图
    float    line_position;                   // 灰度质心 (-5.5~+5.5)
} sensor_data_t;  // ~64 bytes
```

### 全局 volatile (Control 任务直接读)

```c
volatile float        g_current_yaw;    // 累积偏航角 (原始 IMU 值)
volatile sensor_data_t g_sensor_data;   // 完整传感器原始数据
volatile kalman_state_t g_kf_state;     // 卡尔曼滤波后状态
```

---

### 传感器融合: 卡尔曼滤波器

**问题分析:**
- JY61P 偏航角随时间漂移 (陀螺仪零偏积分)
- 编码器距离有滑移误差 (轮子打滑、有效直径变化)
- 弧线段的灰度传感器提供**绝对横向位置参考** (弧线几何已知)
- 顶点 A/B/C/D 坐标已知，经过时可提供**绝对位置修正**
- 直线/对角段无外部参考，纯航位推算

**卡尔曼滤波器设计 (3 状态, M0+ 可承受):**

```c
// 状态向量: [heading(°), gyro_bias(°/s), speed(mm/s)]
typedef struct {
    float heading;       // 融合后航向角 (°)
    float gyro_bias;     // 陀螺仪零偏估计 (°/s)
    float speed;         // 融合后速度 (mm/s)
    
    // 协方差矩阵 (3x3, 对称存储 6 个元素)
    float P[6];          // P11,P12,P13, P22,P23, P33
    
    // 过程噪声 & 测量噪声
    float Q_heading;     // 航向过程噪声
    float Q_bias;        // 零偏随机游走噪声
    float Q_speed;       // 速度过程噪声
    float R_line;        // 灰度线位置测量噪声
    float R_vertex;      // 顶点位置测量噪声
} kalman_state_t;
```

**预测步骤 (每 10ms, Sensor 任务执行):**

```
输入: 陀螺仪角速度 gz (°/s), 编码器速度 enc_speed (mm/s)

1. 状态预测:
   heading = heading + (gz - gyro_bias) * dt     // 陀螺仪积分, 减零偏
   gyro_bias = gyro_bias                          // 零偏缓慢变化
   speed = enc_speed                               // 编码器速度直接作为预测

2. 协方差预测:
   P = F·P·Fᵀ + Q
   其中 F = [[1, -dt, 0], [0, 1, 0], [0, 0, 0]]
   (速度用编码器直接测量, 无动力学模型)

3. 写入全局: g_kf_state.heading, g_kf_state.speed
```

**测量更新 — 弧线段 (灰度传感器, 每 10ms):**

```
当灰度检测到黑线时 (gray != 0):

已知: 弧线几何方程 (圆心、半径已知)
从 line_position (灰度质心 -5.5~+5.5) 计算:
  → 小车在弧线上的切向角 (arc_tangent)
  → 横向偏差 (lateral_error, mm)

测量方程:
  z_heading = arc_tangent    // 弧线切向角 = 期望航向
  H = [1, 0, 0]              // 直接测量 heading

卡尔曼更新:
  y = z_heading - heading
  S = H·P·Hᵀ + R_line
  K = P·Hᵀ / S
  heading += K[0] * y
  gyro_bias += K[1] * y
  speed += K[2] * y
  P = (I - K·H)·P
```

**测量更新 — 顶点 (A/B/C/D, 经过时):**

```
当编码器距离/灰度出线判定到达顶点时:

已知: 顶点绝对坐标 (xi, yi)
从编码器累积距离推算的实际位置与顶点坐标比较:
  → 修正累积距离漂移
  → 修正航向 (从上一段路径方向推断)

这是稀疏但高精度的修正, 可重置部分协方差
```

**直线/对角段 (无外部测量):**

```
仅预测, 无更新 (灰度全白, 无参考线)
→ heading 漂移累积 (gyro_bias 保持上一段估计值)
→ 距离漂移累积 (无法修正)
→ 到达下一顶点时用顶点位置大修正
```

**M0+ 计算量评估 (使用 TI IQMath 定点加速):**

```
3 状态卡尔曼 (Q16.16 定点):
- 预测: ~20 次定点乘加
- 更新: ~30 次定点乘加 (标量测量, 避免矩阵求逆)
- 每 10ms 总计: ~50 次定点运算

IQMath vs 软浮点对比 (Cortex-M0+):
  软浮点 add:  ~50-100 cycles
  软浮点 mul:  ~80-150 cycles
  IQ16 add:    ~1-2 cycles
  IQ16 mul:    ~5-10 cycles (MUL + ASR)
  → 总估算: ~500 CPU cycles, 占 10ms 预算 (800K cycles) 的 ~0.06%
→ 完全可行, 几乎无计算负担
```

**推荐 Q 格式:**
- 角度/航向: Q16.16 (范围 ±32768°, 精度 0.000015°)
- 速度/距离: Q16.16 (范围 ±32768 mm/s, 精度 0.015 mm/s)
- PID 参数: Q16.16
- 协方差矩阵: Q24.8 (更大动态范围)
- 三角函数: 查表法 (sin/cos 预计算表, 40cm 弧线几何固定)

**文件:** `posix_demo/app/app_kalman.c/h` — 卡尔曼滤波器实现, 全部用 IQMath

**灰度辅助动态校准零漂:**

当小车在弧线上稳定循迹时:
- 连续 N 个周期灰度质心始终在中心附近 (|line_position| < 0.5)
- 且 IMU 航向在缓慢漂移
- → 说明陀螺仪零偏估计需要更新
- → 增大该时刻的 measurement update 权重, 加速零偏收敛

---

## 4. LCD 多级菜单系统 (idle+4, 50ms)

### 菜单树

```
MAIN STATUS (默认屏, 5s 无操作自动返回)
└── [CENTER] → 主菜单
    ├── 1. 竞赛模式
    │   ├── 1. Task1: A→B (20分)
    │   ├── 2. Task2: A→B→弧→C→D→弧→A (20分)
    │   ├── 3. Task3: A→C→弧→B→D→弧→A (30分)
    │   ├── 4. Task4: 自动4圈 (30分)
    │   └── 5. 返回
    ├── 2. PID 参数
    │   ├── Speed KP / KI / KD
    │   ├── Steer KP / KD
    │   ├── Heading KP
    │   ├── [保存到 Flash]
    │   └── 返回
    ├── 3. 校准
    │   ├── IMU 偏航归零
    │   ├── IMU 横滚归零
    │   ├── 灰度阈值
    │   ├── [保存到 Flash]
    │   └── 返回
    ├── 4. 按键重映射
    │   ├── Remap UP / DOWN / LEFT / RIGHT / ENTER / BACK
    │   ├── [保存到 Flash]
    │   └── 返回
    ├── 5. 系统设置
    │   ├── 蜂鸣器: 使能/失能
    │   ├── LED 心跳: 开/关
    │   └── 返回
    ├── 6. 系统信息
    │   └── 版本/Flash ID/堆栈余量/运行时间
    └── 7. 返回状态页
```

### 屏幕类型

| 类型 | 说明 |
|------|------|
| SCREEN_STATUS | 主状态页: 偏航角/速度/灰度位图可视化/传感器状态 |
| SCREEN_MENU | 可滚动列表 + 光标 (最多 7 行可见) |
| SCREEN_VALUE_EDIT | 数值编辑 |
| SCREEN_CONFIRM | Y/N 确认对话框 |
| SCREEN_CONTEST | 竞赛运行页: 任务号/圈数/计时/速度/偏航角 |
| SCREEN_INFO | 系统信息 |

### 按键→逻辑方向映射

物理按键通过 `btn_remap[6]` (Flash 存储) 映射为逻辑方向:
- UP: 菜单上/数值增加
- DOWN: 菜单下/数值减小
- LEFT: 返回/数值粗调
- RIGHT: 确认/进入
- ENTER: 确认/进入
- BACK: 返回上一级

### 渲染策略

- 光标移动: 只重绘新旧两行 (节省 ~80% SPI 带宽)
- 全屏刷新: 仅切换屏幕类型时
- 使用现有 `ST7789_drawStringFast` (DMA 逐行)
- 状态页复用 `app_ui` 数值显示

---

## 5. 按键检测服务 (idle+3, 50Hz)

### 新文件: `tasks/task_button.c/h`

- 20ms 周期, 调用扩展后的 `BSP_Button_Scan()`
- 新增事件: HOLD (长按后每 200ms 重复), RELEASE
- 从 Flash 读取 `btn_remap[6]` 物理→逻辑映射
- 发送 `button_event_t {physical, logical, event, tick}` 到 button_queue

### 按键事件类型

```c
#define BTN_EVT_PRESS_DOWN  1   // 按下 (去抖后)
#define BTN_EVT_SHORT       2   // 短按 (<500ms 释放)
#define BTN_EVT_LONG        3   // 长按 (≥500ms 释放)
#define BTN_EVT_HOLD        4   // 持续按住 (每200ms重复)
#define BTN_EVT_RELEASE     5   // 释放
```

---

## 6. 控制系统 (idle+5, 100Hz) — 电赛 H 题

### 策略: 分段控制 — 弧线循迹 + 自由导航

**核心思路**: 只有弧线段有黑线可循，直线/对角段用 IMU + 编码器自由导航。

### 控制模式

```
MODE_LINE_TRACK    — 灰度循迹 (弧线段)
MODE_HEADING_HOLD  — 航向保持 (直线/对角段)
MODE_VERTEX_PAUSE  — 顶点停车 + 声光提示
MODE_IDLE          — 等待指令
MODE_COMPLETE      — 任务完成
```

### 路径规划

**Task 2 路径分解 (1 圈, 顺时针):**
```
1. A→B:     航向 0°(东), 距离 ~100cm                   MODE_HEADING_HOLD
2. B 点:    声光提示                                    MODE_VERTEX_PAUSE
3. B→C:     右弧循迹 (B(50,40)→C(50,-40), 沿右弧下行)   MODE_LINE_TRACK
4. C 点:    声光提示                                    MODE_VERTEX_PAUSE
5. C→D:     航向 180°(西), 距离 ~100cm                  MODE_HEADING_HOLD
6. D 点:    声光提示                                    MODE_VERTEX_PAUSE
7. D→A:     左弧循迹 (D(-50,-40)→A(-50,40), 沿左弧上行) MODE_LINE_TRACK
8. A 点:    声光提示, 任务完成                           MODE_COMPLETE
```

**Task 3 路径分解 (对角):**
```
1. A→C:     航向 -38.7°(东南), 距离 ~128cm             MODE_HEADING_HOLD
2. C 点:    声光提示                                    MODE_VERTEX_PAUSE
3. C→B:     右弧循迹 (C(50,-40)→B(50,40), 沿右弧上行)   MODE_LINE_TRACK
4. B 点:    声光提示                                    MODE_VERTEX_PAUSE
5. B→D:     航向 -128.7°(西南), 距离 ~128cm            MODE_HEADING_HOLD
6. D 点:    声光提示                                    MODE_VERTEX_PAUSE
7. D→A:     左弧循迹 (D(-50,-40)→A(-50,40), 沿左弧上行) MODE_LINE_TRACK
8. A 点:    声光提示, (Task4: 循环4次)                   MODE_COMPLETE
```

**对角段距离计算:** A(-50,40)→C(50,-40): Δx=100, Δy=-80, dist=√(100²+80²)=√16400≈128cm

### 控制循环伪代码 (每 10ms)

```c
void Control_Run(void) {
    // 1. 读取卡尔曼融合状态 (直接读 volatile, 无锁)
    float cur_heading = g_kf_state.heading;    // 融合后航向 (漂移已修正)
    float cur_speed   = g_kf_state.speed;      // 融合后速度
    float cur_dist    = (g_sensor_data.enc1_dist + g_sensor_data.enc2_dist) / 2.0f;
    float line_pos    = g_sensor_data.line_position;
    uint16_t gray     = g_sensor_data.grayscale;

    // 2. 状态机
    switch (mode) {
    case MODE_HEADING_HOLD:
        // 卡尔曼修正后航向保持 + 编码器距离
        heading_err = wrap_180(target_heading - cur_heading);
        steer = HEADING_KP * heading_err;
        if (dist_traveled >= segment_distance) {
            mode = MODE_VERTEX_PAUSE;
            kalman_vertex_update(vertex_coords);  // 顶点位置修正
        }
        break;

    case MODE_LINE_TRACK:
        // 入线检测
        if (!line_detected && gray != 0) {
            line_detected = true;
        }
        if (line_detected) {
            line_err = line_pos;
            steer = STEER_KP * line_err + STEER_KD * (line_err - prev_line_err);
            // 出线检测
            if (gray == 0) line_lost_cnt++; else line_lost_cnt = 0;
            if (line_lost_cnt > LINE_LOST_THRESH) {
                line_detected = false;
                mode = MODE_HEADING_HOLD;
            }
        }
        break;

    case MODE_VERTEX_PAUSE:
        Motor_set(0, 0);
        if (pause_elapsed > 800ms) {
            advance_to_next_segment();
            mode = next_mode;
        }
        break;
    }

    // 3. 速度 PID (内环, 使用卡尔曼融合速度)
    speed_pwm = SPEED_KP * (target_speed - cur_speed);

    // 4. 输出
    left_pwm  = clamp(speed_pwm - steer, -999, 999);
    right_pwm = clamp(speed_pwm + steer, -999, 999);
    Motor_set((int16_t)left_pwm, (int16_t)right_pwm);
}
```

### 灰度质心计算

```c
// 12 位灰度 → 线位置 (-5.5 左 ~ +5.5 右)
// bit[11]=传感器0(最左), bit[0]=传感器11(最右) — 需确认位序
float compute_line_centroid(uint16_t gray) {
    int sum_w = 0, sum_a = 0;
    for (int i = 0; i < 12; i++) {
        if (gray & (1 << (11 - i))) {  // 此通道见黑线
            sum_w += i;
            sum_a++;
        }
    }
    if (sum_a == 0) return 0.0f;           // 无线
    return (float)sum_w / sum_a - 5.5f;    // 居中归零
}
```

### PID 参数 (Flash 可存, LCD 菜单可调)

| 参数 | 默认值 | 说明 |
|------|--------|------|
| SPEED_KP | 2.0 | 速度比例 |
| STEER_KP | 15.0 | 转向比例 (灰度偏差→差速) |
| STEER_KD | 3.0 | 转向微分 |
| HEADING_KP | 8.0 | 航向比例 (角度偏差→差速) |
| TARGET_SPEED | 300 | 目标速度 mm/s |
| LINE_LOST_THRESH | 5 | 出线判定周期数 |

---

## 7. 蜂鸣器驱动 (PB22)

### 新文件: `hardware/hw_buzzer.c/h`

- 无源蜂鸣器，PB22 GPIO 输出
- 高电平响，低电平关
- 菜单中可开关蜂鸣器 (`g_buzzer_enabled`)
- 声光提示模式:
  - 顶点到达: 100ms 响 × 3 次 (间隔 100ms)
  - 任务完成: 500ms 响 + 200ms 停 + 1000ms 响
  - 错误/急停: 持续 500ms 响

```c
void Buzzer_init(void);                         // PB22 输出, 初始 LOW
void Buzzer_set(uint8_t on);                    // 1=响 0=关
void Buzzer_beep(uint16_t dur_ms);              // 阻塞/定时器异步
void Buzzer_pattern(const uint16_t *pat, int n); // 播放模式序列
void Buzzer_enable(bool en);                    // 菜单开关
```

---

## 8. Flash 存储布局

### Sector 0 — 配置数据 (RMW 32 字节块)

```
Offset  Size  Field
0x000   4     magic (0x4D53504D)
0x004   2     version
0x006   1     flags (bit0=calibrated, bit1=remapped, bit2=buzzer_enabled)
0x008   6     btn_remap[6]           // 物理→逻辑按键映射
0x010   4     speed_kp (float)
0x014   4     speed_ki (float)
0x018   4     speed_kd (float)
0x01C   4     steer_kp (float)
0x020   4     steer_kd (float)
0x024   4     heading_kp (float)
0x028   4     target_speed (float)
0x030   4     imu_yaw_offset (float)
0x034   4     imu_roll_offset (float)
0x038   2     gray_threshold (uint16_t)
0x03A   1     buzzer_enabled (uint8_t)
0x040   16    lap_times[4] (uint32_t ms each)
0xFFC   4     checksum (XOR)
```

---

## 9. 文件变更清单

### 新建文件
- `hardware/hw_buzzer.c/h` — 蜂鸣器驱动 (PB22)
- `posix_demo/app/app_menu.c/h` — 菜单状态机 + 6 种屏幕渲染
- `posix_demo/app/app_control.c/h` — 控制算法 (分段 PID + 状态机)
- `posix_demo/app/app_kalman.c/h` — 卡尔曼滤波器 (IQMath 定点实现)
- `posix_demo/tasks/task_button.c/h` — 按键检测服务
- `posix_demo/tasks/task_control.c/h` — 控制任务

### 重大修改
- `posix_demo/main.c` — 全部硬件初始化, 队列先于任务
- `posix_demo/app/app_config.h` — 任务参数 + PID 默认值 + 轨迹参数
- `posix_demo/app/app_flash.c/h` — 扩展配置结构 (+PID/校准/重映射/蜂鸣器开关)
- `posix_demo/tasks/task_sensor.c/h` — 100Hz, 移除 Motor_init/按键, 新增 LED/喂狗/质心/速度, 最高优先级
- `posix_demo/tasks/task_lcd.c/h` — 移除 ST7789_init, 集成 menu 状态机, SPI1 信号量
- `posix_demo/interrupt_priorities.h` — Buzzer/Button 中断项
- `posix_demo/CMakeLists.txt` — 添加新源文件
- `hardware/CMakeLists.txt` — 添加 hw_buzzer.c
- `bsp/bsp_button.c/h` — HOLD/RELEASE 事件

### 删除文件
- `task_led.c/h` — 合并入 task_sensor
- `task_motor.c/h` — 替代为 task_control
- `task_flash.c/h` — 测试任务
- `task_setup.c/h` — 替代为 LCD 菜单按键重映射向导

### 保持不变
- `bsp/*` (除 bsp_button 扩展)
- `hardware/hw_{jy61p,nchd12,st7789,w25q128,motor}.c/h`
- `lib/*`, FreeRTOS 内核, SysConfig 生成文件

---

## 10. 实施阶段

### Phase 1: 基础重构
1. 移动 Motor_init/ST7789_init 到 main.c
2. Sensor 100Hz + LED 心跳 + 喂狗, 提升到最高优先级
3. 删除 task_led.c/h

### Phase 2: 按键服务
4. 创建 task_button.c/h
5. 扩展 bsp_button.c (HOLD/RELEASE)
6. main.c 创建 TaskButton + button_queue

### Phase 3: LCD 菜单
7. 创建 app_menu.c/h
8. 更新 task_lcd.c 消费 button_queue
9. 实现所有屏幕 + 按键重映射向导 + SPI1 信号量

### Phase 4: Flash 存储扩展
10. 扩展 app_flash.c/h (PID/校准/重映射/蜂鸣器开关)

### Phase 5: 蜂鸣器
11. 创建 hw_buzzer.c/h (PB22)

### Phase 6: 卡尔曼滤波
12. 创建 app_kalman.c/h — Q16.16 定点 3 状态 EKF
13. 集成到 Sensor 任务: 预测每 10ms, 弧线测量更新, 顶点测量更新

### Phase 7: 控制系统
14. 创建 app_control.c/h + task_control.c/h
15. 实现 MODE_LINE_TRACK + MODE_HEADING_HOLD + 状态机 + 声光
16. 删除 task_motor.c/h, task_flash.c/h, task_setup.c/h

### Phase 8: 集成测试
17. 编译验证 → 菜单导航 → Flash 持久化 → 竞赛任务实地测试 → PID 在线调优

---

## 11. 风险与缓解

| 风险 | 缓解 |
|------|------|
| 12KB 堆溢出 | 删除 task_flash 释放其栈; sensor_queue 静态分配; LCD 栈可降为 1280 |
| SPI1 竞争 | 二值信号量 100ms 超时; Flash 擦除忙等待期间释放信号量 |
| Tickless 死锁 | 所有阻塞操作用有限超时, 不用 portMAX_DELAY |
| 编码器 ISR 高速丢脉冲 | 2 倍解码, 最高速 ~5300 脉冲/秒/编码器, ISR ~1.25μs |
| 菜单渲染太慢 | 只重绘变化行; DMA 并行传输 |
| M0+ 浮点运算慢 | 全部控制/滤波用 TI IQMath Q16.16 定点, 比软浮点快 10-20 倍 |
| JY61P 偏航漂移 | 卡尔曼滤波在线估计 gyro_bias; 弧线灰度 + 顶点坐标周期性修正 |
| 编码器打滑 | 卡尔曼速度融合; 顶点已知距离校零 |
| 灰度传感器位序不确定 | 实测 12 通道对应物理位置, 在 app_config.h 中配置映射表 |

---

## 验证

1. 编译 0 错误 0 警告
2. 上电: Splash → 状态页, LED 500ms 闪烁, 看门狗运行
3. 菜单: 全菜单可导航, PID 值可编辑, 断电重启后值保持
4. 按键重映射: 修改后保存重启生效
5. 蜂鸣器: PB22 顶点声光提示, 菜单可开关
6. 竞赛: Task1-4 各路径正常运行, 弧线不脱离黑线
7. Sensor 100Hz: 数据帧率稳定 (逻辑分析仪/UART 时间戳验证)
8. 内存: 堆余量 >1KB, 各任务栈高水位 <60%
