# MSPM0G3507 智能小车 — 2024 电赛 H 题

基于 TI MSPM0G3507 (Cortex-M0+ @80MHz) + FreeRTOS v11.2.0 的自动驾驶小车固件。

**当前版本: v2.1 — 完赛 (4圈全通)**

## 硬件

| 模块 | 型号 | 接口 |
|------|------|------|
| MCU | MSPM0G3507 LQFP-64 | Cortex-M0+ @80MHz |
| IMU | JY61P | I2C0 (PA0/PA1, 1MHz, DMA RX) |
| 灰度 | NC-HD12 12路 (PCA9555) | I2C1 (PB3/PA29, 100kHz, DMA RX) |
| Flash | W25Q128 16MB | SPI1 (PB6 CS, DMA RX 读) |
| LCD | ST7789 170×320 | SPI1 (PB14 CS, 10MHz, DMA TX) |
| 电机 | TB6612NG ×2 | TIMA1 PWM 40kHz (PA16/PA17) + PA12-15 方向 |
| 编码器 | GMR 500PPR 1:20 | GPIOA 上升沿中断 (PA26-31) |
| 蜂鸣器 | 有源 | PB22 |
| 按键 | 6 键 (五向 + BTN) | GPIO 轮询 (20ms 去抖) |

## 工程结构

```
├── bsp/              # BSP 层: SPI/I2C/UART/GPIO/按键 (纯硬件, 无 RTOS)
├── hardware/         # 外设驱动: motor/st7789/jy61p/nchd12/w25q128/buzzer
├── hal/              # HAL 层: SPI 递归互斥锁
├── app/              # 应用层
│   ├── app_config.h          # 全局常量 (赛道几何/PID默认值/任务参数)
│   ├── app_control.c/h       # 3 层级联 PID + 竞赛状态机 + LINE_SEEK 寻线
│   ├── app_pid.c/h           # PID 控制器 (抗积分饱和)
│   ├── app_kalman.c/h        # 5 状态 EKF 传感器融合
│   ├── app_motor.c/h         # 电机速度闭环 (per-motor PID)
│   ├── app_menu.c/h          # LCD 多级菜单 (6 屏 + 缓变渲染)
│   ├── app_flash.c/h         # Flash 配置持久化 (checksum)
│   └── app_slip.c/h          # 打滑检测 (IMU accel vs 编码器)
├── tasks/            # FreeRTOS 任务
│   ├── task_sensor.c/h       # 传感器采集 (100Hz, prio 6)
│   ├── task_control.c/h      # 控制循环 (100Hz 竞赛, prio 5)
│   ├── task_lcd.c/h          # LCD 渲染 (20Hz, prio 4)
│   └── task_button.c/h       # 按键扫描 (50Hz, prio 3)
├── lib/              # 字库 + Paint 绘图
├── posix_demo/       # main.c + SysConfig + FreeRTOS 配置
├── firmware.hex      # 烧录文件 (Intel HEX)
└── firmware.bin      # 烧录文件 (裸二进制)
```

## 任务架构

| 任务 | 优先级 | 栈 | 周期 |
|------|--------|-----|------|
| Sensor | idle+6 | 640w | 10ms (100Hz) |
| Control | idle+5 | 768w | 10ms (100Hz, 竞赛模式) |
| LCD | idle+4 | 1024w | 50ms (20Hz) |
| Button | idle+3 | 128w | 20ms (50Hz) |

## 控制流程

```
CTRL_POSITION ──距离到──→ CTRL_LINE_SEEK ──单线居中──→ CTRL_LINE_TRACK
(航向保持)              (慢速寻线+渐进航向)          (灰度循迹)
                                                         │
                                              CTRL_VERTEX_PAUSE ←── 出线
                                              (声光+Kalman修正)
                                                     │
                                                     ↓
                                              CTRL_POSITION (下一段)
```

### LINE_SEEK 寻线过渡
- 距离到达后慢速前探 (80mm/s)
- 航向从当前角渐进至弧线切向角 (800ms)
- 灰度检测到单黑块且居中 → 切入 LINE_TRACK
- 遇线时用线位置修正航向, 对抗 JY61P 漂移
- 150mm 超距兜底

## 快速开始

### 编译

```
工具链: TI Arm Clang 4.0.4.LTS
SDK:    MSPM0 SDK 2.10.00.04
CMake:  3.21+ / Ninja

cd build
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=../cmake/ti_ticlang.cmake
ninja
```

### 烧录

用 `firmware.hex` (CCS / UniFlash) 或 `firmware.bin` (J-Link / OpenOCD, 地址 0x00000000)。

### 串口命令

115200-8N1, 单字节:

| 键 | 功能 |
|---|------|
| `1`-`4` | 启动竞赛 Task 1-4 |
| `0` / `space` | 急停 |
| `d` | Debug 单步模式 |
| `c` | Debug 继续 |

### LCD 菜单

```
STATUS 仪表盘 → [ENTER] → 主菜单
├── 1. Contest Mode   (Task1-4)
├── 2. PID Parameters  (速度/航向/转向 PID 在线调参)
├── 3. Calibration     (IMU 归零 / 灰度阈值)
├── 4. Button Remap    (6 键物理→逻辑重映射)
├── 5. System Settings  (蜂鸣器 / LED / 电机方向)
├── 6. System Info      (版本 / 堆栈 / 卡尔曼状态)
└── 7. Return
```

## Flash 配置

W25Q128 Sector 0, XOR checksum:

| 参数 | 说明 |
|------|------|
| PID 参数 (7 floats) | 速度/转向/航向 Kp Ki Kd |
| 按键重映射 (6 bytes) | 物理→逻辑方向 |
| 电机方向 | A/B 正反转配置 |
| 编码器极性 | Enc1/Enc2 计数方向 |
| IMU 校准偏移 | Yaw/Roll/Pitch 零点 |
| 蜂鸣器/LED 开关 | Flags 位 |

## 赛道几何

```
       A(-500,400) ─── 1000mm ─── B(500,400)
          |                          |
      LEFT ARC                   RIGHT ARC
   (x+500)²+y²=400²         (x-500)²+y²=400²
   R=400, 圆心(-500,0)       R=400, 圆心(500,0)
          |                          |
       D(-500,-400) ─── 1000mm ─── C(500,-400)
```

黑色弧线宽 1.8cm, 直线段无标记。对角线 A↔C 和 B↔D 也无线。

## 竞赛任务

| 任务 | 路径 | 时限 |
|------|------|------|
| Task 1 | A → B | ≤15s |
| Task 2 | A→B→右弧→C→D→左弧→A (1圈) | ≤30s |
| Task 3 | A→C→右弧→B→D→左弧→A (对角) | ≤40s |
| Task 4 | Task3 路径 ×4 圈 | 竞速 |

## 版本

| 版本 | 说明 |
|------|------|
| v2.1 | 完赛: 4圈全通, LINE_SEEK 寻线过渡, HAL_SPI 互斥, DMA BSP |
| v2.0 | 3层级联 PID + 5状态 EKF + 多级菜单 |
| v1.0 | P 控制器 + 偏航角恢复 + 基础 Flash 存储 |

## License

MIT
