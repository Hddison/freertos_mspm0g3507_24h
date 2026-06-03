# FreeRTOS MSPM0G3507 — 智能小车 BSP 工程

基于 TI MSPM0G3507 (Cortex-M0+ @ 80MHz) + FreeRTOS 11.2.0 的双轮差速小车底层工程。

## 硬件

| 模块 | 型号 | 接口 | 说明 |
|------|------|------|------|
| MCU | MSPM0G3507 | — | 80MHz, 128KB Flash, 32KB SRAM |
| IMU | JY61P | I2C0 (PA0/PA1) | 6 轴姿态 (Roll/Pitch/Yaw), 1MHz FastPlus |
| 灰度 | NC-HD12 (PCA9555) | I2C1 (PB3/PA29) | 12 路数字灰度 |
| 电机 | TB6612NG ×2 | PWM + GPIO | 双路 H 桥驱动 |
| 编码器 | GMR 500PPR ×2 | GPIO 中断 | 双边沿解码, 距离换算 |
| 屏幕 | ST7789 1.9" | SPI1 (PB7/8/9) | 170×320 竖屏, 40MHz DMA |
| 按键 | 五向开关 + BUTTON | GPIO 输入 | 6 键轮询 (短按/长按) |
| LED | 板载 LED | PB20 | 500ms 闪烁 |

## 环境

| 工具 | 路径 |
|------|------|
| TI ARM CLANG | `D:\ti\ccs2050\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS` |
| MSPM0 SDK | `C:\ti\mspm0_sdk_2_10_00_04` |

路径不同时修改 `cmake/ti_ticlang.cmake`、`posix_demo/CMakeLists.txt`、`freertos_builds_*/CMakeLists.txt` 中的路径变量。

## 构建 & 烧录

```bash
# 配置
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/ti_ticlang.cmake -GNinja -B build

# 编译
cmake --build build

# 烧录 (probe-rs)
probe-rs download --chip MSPM0G3507 build/posix_demo/posix_demo.out
```

## 工程结构

```
├── bsp/                             # BSP 层 (纯 DriverLib 封装)
│   ├── bsp.h                        #   聚合头文件
│   ├── bsp_i2c.c/h                  #   I2C (DMA + 超时保护 + 总线恢复)
│   ├── bsp_spi.c/h                  #   SPI (超时保护)
│   ├── bsp_uart.c/h                 #   UART (阻塞 TX + 超时 RX)
│   ├── bsp_system.c/h               #   延时 + 系统时钟
│   └── bsp_button.c/h               #   按键轮询 (短按/长按)
│
├── hardware/                        # 外设驱动层
│   ├── hw_st7789.c/h                #   LCD 驱动 (SPI DMA 快速字符串)
│   ├── hw_jy61p.c/h                 #   JY61P IMU (累积偏航角解卷绕)
│   ├── hw_nchd12.c/h                #   NC-HD12 灰度 (PCA9555 I2C GPIO)
│   └── hw_motor.c/h                 #   TB6612 电机 + GMR 编码器
│
├── lib/                             # 通用库
│   ├── gui_paint.c/h                #   Paint 绘图引擎 (Waveshare)
│   └── fonts/                       #   5 种 ASCII 字体 (8/12/16/20/24)
│
├── posix_demo/                      # 应用层
│   ├── main.c                       #   入口: 系统初始化 + 创建任务 (~60 行)
│   ├── interrupt_priorities.h       #   NVIC 中断优先级分配
│   ├── posix_demo.syscfg            #   SysConfig 硬件配置
│   ├── mspm0g3507.cmd               #   链接脚本
│   ├── app/
│   │   ├── app_config.h             #   任务常量 (周期/优先级/栈/队列)
│   │   └── app_ui.c/h               #   UI 工具 (ftoa, DMA 数值绘制)
│   ├── tasks/
│   │   ├── task_led.c/h             #   LED 任务 (2Hz)
│   │   ├── task_sensor.c/h          #   传感器任务 (20Hz, 生产者)
│   │   ├── task_lcd.c/h             #   LCD 任务 (20Hz, 消费者)
│   │   └── task_motor.c/h           #   电机任务 (100Hz, 占位)
│   ├── syscfg/
│   │   └── ti_msp_dl_config.c/h     #   SysConfig 生成的外设配置
│   └── freertos/ticlang/
│       └── startup_mspm0g350x_ticlang.c  # 中断向量表 + 启动代码
│
├── freertos_builds_*/               # FreeRTOS 内核静态库
│   ├── FreeRTOSConfig.h             #   FreeRTOS 内核配置
│   └── CMakeLists.txt
│
├── cmake/
│   └── ti_ticlang.cmake             # TI ARM CLANG 工具链文件
│
└── build/                           # CMake 构建输出 (gitignore)
```

## 命名规范

| 层 | 模式 | 示例 |
|------|-------|-------|
| BSP | `BSP_{Module}_{action}` | `BSP_I2C_write`, `BSP_Button_Scan` |
| 硬件 | `{Device}_{action}` | `ST7789_init`, `Motor_set` |
| 库 | 保持上游命名 | `Paint_NewImage` (Waveshare) |
| 应用 | `Task{Name}_{action}` | `TaskSensor_create`, `TaskLcd_create` |

## 任务架构

```
                         vTaskStartScheduler()
                                 │
          ┌──────────────────────┼──────────────────────┐
          │                      │                      │
     TaskLed               TaskSensor              TaskMotor
     (prio 1, 2Hz)         (prio 2, 20Hz)          (prio 4, 100Hz)
          │                      │                      │
    DL_GPIO_toggle         I2C0→JY61P_readAngle     Motor_set() [占位]
                           I2C1→NCHD12_read         PID 控制 [TODO]
                                 │
                          sensor_data_t 队列 (mailbox, 长度 1)
                                 │
                            TaskLcd
                            (prio 3, 20Hz)
                                 │
                     SPI1 DMA → ST7789 渲染
                     BSP_Button_Scan 按键处理
```

| 任务 | 文件 | 栈 (字) | 周期 | 模式 | 职责 |
|------|------|---------|------|------|------|
| LED | `task_led.c` | 128 | 2Hz | 周期 | PB20 闪烁 |
| Sensor | `task_sensor.c` | 512 | 20Hz | 生产者 | JY61P + NCHD12 + 编码器 + 按键, 数据推入 mailbox 队列 |
| LCD | `task_lcd.c` | 1024 | 20Hz | 消费者 | 阻塞接收队列数据, DMA 刷新角度/距离/灰度 |
| Motor | `task_motor.c` | 512 | 100Hz | 控制 | PID 闭环控制 (占位, 待开发) |

## 中断优先级

Cortex-M0+ 仅 2 位优先级 (4 级, 0=最高)。详见 `posix_demo/interrupt_priorities.h`。

| 优先级 | 值 | 外设 | 说明 |
|--------|-----|------|------|
| 0 (最高) | 0x00 | SysTick/PendSV/SVC | FreeRTOS 内核独占, **禁止** FromISR API |
| 1 | 0x40 | DMA, SPI1 | 高速通信, 可调用 FromISR API |
| 2 | 0x80 | I2C0, I2C1, GPIOA(编码器), TIMG7/8 | 传感器/外设, 可调用 FromISR API |
| 3 (最低) | 0xC0 | UART0, TIMA1(未使用) | 调试 |

## FreeRTOS 关键配置

| 参数 | 值 |
|------|-----|
| CPU_CLOCK | 80 MHz |
| TICK_RATE | 1000 Hz |
| 抢占式 | 开启 |
| 时间片 | 关闭 |
| Tickless Idle | 开启 |
| 最大优先级 | 10 |
| 堆大小 | 12 KB (heap_4) |
| 栈溢出检测 | Method 2 |
| 静态/动态分配 | 均支持 |

## DMA 分配

| 通道 | 外设 | 用途 |
|------|------|------|
| CH0 | SPI1 TX | LCD 区域填充 + 字符渲染 |
| CH1 | I2C1 RX | NCHD12 灰度读取 |
| CH2 | I2C0 RX | JY61P 角度读取 |
| CH3 | I2C0 TX | (备用) |

## 按键功能

| 事件 | 动作 |
|------|------|
| 短按 | 编码器清零 |
| 长按 | 预留 |

## 引脚连接

详见 `posix_demo/posix_demo.syscfg` (SysConfig GUI)。

## 内存占用

| 段 | 大小 | 总量 | 占比 |
|----|------|------|------|
| Flash (text) | 44 KB | 128 KB | 34% |
| SRAM (bss+data) | 18 KB | 32 KB | 56% |

## License

MIT
