# FreeRTOS MSPM0G3507 — 智能小车 BSP 工程 (v1.0)

基于 TI MSPM0G3507 (Cortex-M0+ @ 80MHz) + FreeRTOS 的全硬件验证 BSP 工程。

## 架构

```
main.c/RTOS tasks → HW (hw_*.c) → HAL (hal_spi) → BSP (bsp_*.c)
                                    ↕ FreeRTOS mutex
```

| 层 | 职责 | RTOS 依赖 |
|----|------|-----------|
| **BSP** (`bsp/`) | 纯 DriverLib 封装, SPI/I2C/UART/GPIO | 无 |
| **HAL** (`hal/`) | RTOS 感知, SPI 总线互斥锁 | FreeRTOS mutex |
| **HW** (`hardware/`) | 外设驱动, 调用 HAL+BSP | 间接 (通过 HAL) |
| **Lib** (`lib/`) | 通用工具 + GUI 绘制 | 无 |

## 硬件

| 模块 | 型号 | 接口 | 状态 |
|------|------|------|:--:|
| MCU | MSPM0G3507 | — | ✅ |
| IMU | JY61P | I2C0 (PA0/PA1, 1MHz) | ✅ |
| 灰度 | NC-HD12 (PCA9555) | I2C1 (PB3/PA29, 100kHz) | ✅ |
| Flash | W25Q128 16MB | SPI1 (CS=PB6) | ✅ |
| 屏幕 | ST7789 1.9" | SPI1 (CS=PB14, 10MHz) | ✅ |
| 电机×2 | TB6612NG | PWM (PA16/17) + GPIO | ✅ |
| 编码器×2 | GMR 500PPR | GPIOA 双边沿中断 | ✅ |
| 蜂鸣器 | 有源 | PB22 | ✅ |
| 按键 | 6 键 (五向+BTN) | GPIO 轮询 | ✅ |
| LED | 板载 | PB20 | ✅ |
| UART | 调试串口 | UART0 (PA10/11, 115200) | ✅ |

## 快速开始

```bash
# 构建
cd posix_demo/build
cmake .. -G Ninja && ninja

# 烧录 (J-Link / OpenOCD / probe-rs)
```

## 上电自检

上电后自动运行 8 项硬件自检, 结果输出串口+LCD:

```
[0] UART+LED: PASS
[0] BUTTONS:  PASS
[0] BUZZER:   PASS
[0] FLASH:    PASS
[0] LCD:      PASS
[0] IMU:      PASS
[0] GRAYSCALE:PASS
[0] MOTOR+ENC:PASS
[0] ALL DONE:
```

自检通过后进入 **传感器仪表盘**, 20Hz 刷新 IMU 角度/陀螺/加速度 + 灰度 + 编码器计数/转速。

## 工程结构

```
├── bsp/                         # BSP 层 (纯硬件, 无 RTOS)
│   ├── bsp_spi.c/h              #   SPI1 DMA (initChannel/FIFO drain)
│   ├── bsp_uart.c/h             #   UART0 DMA TX/RX + 阻塞模式
│   ├── bsp_i2c.c/h              #   I2C0/I2C1 DMA + CPU 读写
│   ├── bsp_system.c/h           #   延时 + 系统时钟
│   └── bsp_button.c/h           #   按键状态机 (去抖/短按/长按)
│
├── hal/                         # HAL 层 (RTOS 感知)
│   └── hal_spi.c/h              #   SPI1 总线互斥 (FreeRTOS mutex)
│
├── hardware/                    # 外设驱动层
│   ├── hw_w25q128.c/h           #   W25Q128 Flash (CPU写+DMA读)
│   ├── hw_st7789.c/h            #   ST7789 LCD (DMA填充+文字)
│   ├── hw_jy61p.c/h             #   JY61P IMU (DMA burst读取)
│   ├── hw_nchd12.c/h            #   NC-HD12 灰度 (PCA9555 I2C)
│   ├── hw_motor.c/h             #   TB6612 电机 + 编码器 ISR + 10ms 转速
│   └── hw_buzzer.c/h            #   有源蜂鸣器
│
├── lib/                         # 通用库
│   ├── gui_paint.c/h            #   Paint 绘图引擎
│   ├── util.c/h                 #   轻量 itoa/ftoa/bits12 (无标准库)
│   └── fonts/                   #   5 种 ASCII 字体 (8/12/16/20/24)
│
├── posix_demo/                  # 应用层
│   ├── main.c                   #   自检 + 仪表盘
│   ├── posix_demo.syscfg        #   SysConfig 硬件配置
│   ├── mspm0g3507.cmd           #   链接脚本
│   ├── syscfg/                  #   SysConfig 生成文件
│   └── freertos/ticlang/        #   启动代码 + 向量表
│
├── freertos_builds_*/           # FreeRTOS 静态库
├── TECH_NOTES.md                # 硬件调试技术笔记
└── HW_TEST.md                   # 逐模块测试计划
```

## 关键设计决策

| 决策 | 原因 |
|------|------|
| Flash 写用 CPU, 不用 DMA TX | MSPM0 SPI DMA TX 有 3 字节管道偏移 |
| Flash 读用 DMA RX | DMA RX 完美工作 |
| LCD 每行擦除+重绘 | 避免整屏刷新闪屏, Font8 5×8 紧凑 |
| DMA 通道每次 initChannel | SPI1 LCD+Flash 复用同一 DMA 通道 |
| LCD DMA 后 drain TX+RX FIFO | 防止残留数据污染下一次 Flash SPI 操作 |
| HAL_SPI_init() 调度器前调用 | 避免惰性初始化的 FreeRTOS mutex 竞态 |
| ISR 放 hw_motor.c (非 main.c) | 静态库链接正常 (GROUP1+TIMA0 均工作) |
| util_itoa 用 uint32_t 安全取负 | 避免 INT32_MIN 溢出 (编码器 32-bit torn read) |

## 总线布局

| 总线 | 设备 | 互斥 |
|------|------|:--:|
| SPI1 (10MHz) | W25Q128 + ST7789 | HAL mutex |
| I2C0 (1MHz) | JY61P | — (独占) |
| I2C1 (100kHz) | NCHD12 | — (独占) |
| UART0 (115200) | 调试台 | — (独占) |

## DMA 分配

| 通道 | 外设 | 用途 |
|------|------|------|
| CH0 (DMA_CH2) | UART TX | 串口 DMA 发送 |
| CH1 (DMA_CH1) | UART RX | 串口 DMA 接收 |
| CH2 (DMA_SPI_LCD_RX) | SPI1 RX | Flash DMA 读 |
| CH3 (DMA_SPI_LCD_TX) | SPI1 TX | LCD DMA 填充+文字 |
| CH4 (DMA_CH0) | I2C1 RX | 灰度 DMA 读 |
| CH5 (DMA_I2C_RX) | I2C0 RX | IMU DMA 读 |
| CH6 (DMA_I2C_TX) | I2C0 TX | (备用) |

## FreeRTOS 关键配置

| 参数 | 值 |
|------|-----|
| CPUCLK | 80 MHz |
| Tick | 1000 Hz |
| 堆 | 12 KB (heap_4) |
| 栈溢出检测 | Method 2 |
| 最小栈 | 256 words |
| 任务栈 | 512-2048 words |
| Mutex/Sem | 已启用 |

## 技术笔记

详见 [TECH_NOTES.md](TECH_NOTES.md) — SPI+DMA 配置、FreeRTOS 常见坑、各模块调试记录。

## License

MIT
