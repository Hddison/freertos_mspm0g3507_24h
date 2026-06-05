# MSPM0G3507 智能小车 — 2024 电赛 H 题 (v2.0)

基于 TI MSPM0G3507 (Cortex-M0+ @80MHz) + FreeRTOS v11.2.0 的自动驾驶小车工程。

## 架构

```
Task Button (50Hz) → button_queue(8) → Task LCD (20Hz) → cmd_queue(4) → Task Control (100Hz)
                                                                              ↑ volatile
                                          Task Sensor (100Hz) ──── g_kf_state ──→
```

| 层 | 目录 | 职责 |
|----|------|------|
| **BSP** | `bsp/` | 纯 DriverLib 封装 (SPI/I2C/UART/GPIO/按键状态机) |
| **HAL** | `hal/` | SPI 递归互斥锁 (Flash+LCD 嵌套调用) |
| **HW** | `hardware/` | 外设驱动 (IMU/灰度/Flash/LCD/电机/编码器/蜂鸣器) |
| **App** | `app/` | 应用算法 (PID/Kalman/菜单/控制/Flash配置/打滑检测) |
| **Tasks** | `tasks/` | 4 个 FreeRTOS 任务 (Sensor/Control/LCD/Button) |
| **Lib** | `lib/` | Paint 绘图引擎 + util + 5 种 ASCII 字体 |

## 硬件

| 模块 | 型号 | 接口 | 
|------|------|------|
| MCU | MSPM0G3507 | Cortex-M0+ @80MHz |
| IMU | JY61P (6轴+姿态融合) | I2C0 (PA0/PA1, 1MHz, DMA) |
| 灰度 | NC-HD12 12路 (PCA9555) | I2C1 (PB3/PA29, 100kHz, DMA) |
| Flash | W25Q128 16MB | SPI1 (CS=PB6, DMA) |
| 屏幕 | ST7789 1.9" 170×320 | SPI1 (CS=PB14, 10MHz, DMA) |
| 电机×2 | TB6612NG | TIMA1 PWM 40kHz (PA16/PA17) |
| 编码器×2 | GMR 500PPR 1:20 | GPIOA 双边沿中断 + TIMA0 10ms 采样 |
| 蜂鸣器 | 有源 | PB22 |
| 按键 | 6 键 (五向+BTN) | GPIO 轮询 (20ms 去抖) |

### 安装约定
- JY61P: Y- 轴 = 前进方向, X+ 轴 = 左方向, Z 轴朝上
- Motor A = 右轮, Motor B = 左轮 (可在菜单中互换)
- 编码器正向 = 车轮前进方向

## 快速开始

```bash
cd posix_demo/build
cmake .. -G Ninja && ninja
# 烧录 posix_demo.out
```

串口 115200 baud, 单字节命令:

| 键 | 功能 |
|---|------|
| `1`-`4` | 电机 A/B 正反转测试 |
| `5`-`6` | 双轮前进/后退 |
| `0` | 全停 |
| `e` | 读取编码器 |

## 功能

### 控制架构
```
位置环 (外环) → target_speed + curvature
    ├→ 速度 PID (内环) → base_pwm
    └→ 曲率×航向 PD → steer
              ↓
    left = base_pwm - steer, right = base_pwm + steer
```

### 位置解算
5 状态扩展卡尔曼滤波器: `[x(mm), y(mm), θ(rad), v(mm/s), ω(rad/s)]`
- 预测: 编码器里程计 + IMU 陀螺仪 (100Hz)
- 更新: 灰度横向纠偏 + 顶点绝对位置修正
- M0+ 优化: 所有测量更新用标量形式, 无矩阵求逆

### LCD 多级菜单
```
STATUS 仪表盘 → [CENTER] → 主菜单
├── 1. Contest Mode   (Task1-4 竞赛路径)
├── 2. PID Parameters  (三环 PID 在线调参 + 恢复出厂)
├── 3. Calibration     (IMU 归零 / 灰度阈值)
├── 4. Key Remapping   (6 键物理→逻辑重映射)
├── 5. System Settings  (蜂鸣器/LED/电机配置)
├── 6. System Info      (版本/堆栈/Kalman状态)
└── 7. Return
```
- 6 种屏幕: STATUS / MENU / VALUE_EDIT / CONFIRM / CONTEST / INFO
- 渲染优化: 缓变重绘, 只刷新变化行 (~3ms/帧)
- Auto-return: 5 秒无操作自动回 STATUS

### Flash 配置
W25Q128 Sector 0, 含 checksum 验证:
- PID 参数 (7 个 float)
- 按键重映射 (6 字节)
- 电机方向 & 编码器极性
- IMU 校准偏移
- 蜂鸣器/LED 开关
- 圈速记录

## 工程结构

```
├── bsp/                    # BSP 层 (纯硬件, 无 RTOS)
├── hal/                    # HAL 层 (SPI 递归互斥锁)
├── hardware/               # 外设驱动 (6 个模块)
├── lib/                    # 通用库 (Paint + util + fonts)
├── app/                    # 应用层算法
│   ├── app_config.h        #   全局常量 (赛道几何/PID默认值/任务参数)
│   ├── app_pid.c/h         #   可复用 PID 控制器 (抗积分饱和, D-on-meas)
│   ├── app_kalman.c/h      #   5 状态 EKF (标量更新, 无矩阵求逆)
│   ├── app_control.c/h     #   三环级联 + 竞赛状态机 + 路径表
│   ├── app_menu.c/h        #   多级菜单 (6 屏, 哨兵安全, 缓变渲染)
│   ├── app_flash.c/h       #   配置持久化 (checksum 验证)
│   ├── app_slip.c/h        #   打滑检测 (IMU 加速度计 vs 编码器)
│   └── app_motor_test.c/h  #   电机测试状态机
├── tasks/                  # RTOS 任务
│   ├── task_button.c/h     #   按键扫描 (50Hz, idle+3)
│   ├── task_lcd.c/h        #   菜单渲染 (20Hz, idle+4)
│   ├── task_sensor.c/h     #   传感器融合 (100Hz, idle+6)
│   └── task_control.c/h    #   控制循环 (100Hz idle, 竞赛 100Hz)
├── posix_demo/             # 应用入口 + SysConfig
│   ├── main.c              #   16 步硬件初始化 + 队列/任务创建
│   ├── posix_demo.syscfg   #   SysConfig 硬件配置
│   └── syscfg/             #   生成代码
└── freertos_builds_*/      # FreeRTOS 内核库
```

## 任务参数

| 任务 | 优先级 | 栈 | 周期 | 状态 |
|------|--------|-----|------|------|
| Sensor | idle+6 | 640w | 10ms | 常跑 |
| Control | idle+5 | 768w | -- | 空闲阻塞, 竞赛 100Hz |
| LCD | idle+4 | 1024w | 50ms | 常跑 |
| Button | idle+3 | 128w | 20ms | 常跑 |

## 关键修复记录

| 问题 | 根因 | 修复 |
|------|------|------|
| LCD 黑屏/按键无响应 | SPI 普通锁嵌套死锁 | → 递归互斥锁 |
| HardFault | UART RX 中断使能无 ISR + tickless idle | → NVIC_DisableIRQ + 关闭 tickless |
| 菜单按键失效 | 57ms DMA 清屏饿死 Button 任务 | → 分块清屏 + 底部不清 |
| 屏幕切换重叠 | switch 不清屏 + 子菜单不检测 menu 变化 | → 检测 menu 指针变化 |
| 菜单数组越界 | while(p->title) 无哨兵读到相邻数组 | → 所有数组加 NULL 哨兵 |
| PID 保存不持久 | control_init 在 load_flash 之后调用 | → Control 任务启动后再 load |
| 按键重映射保存破坏 | g_btn_remap_tmp 未初始化全零 | → 仅在重映射菜单保存时应用 tmp |
| 编码器方向错误 | 装配方向与代码假设不一致 | → Flash 存储 polarity 可配置 |
| 电机方向错误 | Motor B 正 PWM=后退 | → 菜单 Motor Config 可切换 |

## 编译

- 工具链: TI Arm Clang 4.0.4.LTS
- CMake 3.21+, Ninja
- SDK: MSPM0 SDK 2.10.00.04
- 优化: -O2, -Wall

## License

MIT
