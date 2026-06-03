# FreeRTOS MSPM0G3507 — 智能小车 BSP 工程

基于 TI MSPM0G3507 (Cortex-M0+ @ 80MHz) + FreeRTOS 11.2.0 的双轮差速小车底层工程。

## 硬件

| 模块 | 型号 | 接口 | 说明 |
|------|------|------|------|
| MCU | MSPM0G3507 | — | 80MHz, 128KB Flash, 32KB SRAM |
| IMU | JY61P | I2C0 (PA0/PA1) | 6 轴姿态 (Roll/Pitch/Yaw) |
| 灰度 | NC-HD12 (PCA9555) | I2C1 (PB3/PA29) | 12 路数字灰度 |
| 电机 | TB6612NG ×2 | PWM + GPIO | 双路 H 桥驱动 |
| 编码器 | GMR 500PPR ×2 | GPIO 中断 | 正交编码, 2x 解码 |
| 屏幕 | ST7789 1.9" | SPI1 (PB7/8/9) | 170×320 竖屏, 40MHz |
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
├── bsp/                         # BSP 层 (纯 DL 封装)
│   ├── bsp_i2c.c/h              # I2C (DMA + 超时保护)
│   ├── bsp_spi.c/h              # SPI
│   ├── bsp_uart.c/h             # UART (阻塞式调试输出)
│   ├── bsp_system.c/h           # 延时 + 系统时钟
│   └── bsp_button.c/h           # 按键轮询 (短按/长按)
│
├── hardware/                    # 外设驱动
│   ├── hw_st7789.c/h            # LCD 驱动 + 快速字符串
│   ├── hw_jy61p.c/h             # JY61P IMU 驱动
│   ├── hw_nchd12.c/h            # NC-HD12 灰度 (PCA9555)
│   └── hw_motor.c/h             # TB6612 电机 + GMR 编码器
│
├── lib/                         # 通用库
│   ├── gui_paint.c/h            # Paint 绘图引擎 (Waveshare)
│   └── fonts/                   # 5 种 ASCII 字体 (8/12/16/20/24)
│
├── posix_demo/                  # 应用层
│   ├── main.c                   # 入口 + 所有任务
│   ├── posix_demo.syscfg        # SysConfig 配置
│   ├── mspm0g3507.cmd           # 链接脚本
│   └── freertos/ticlang/        # 启动文件
│
├── freertos_builds_*/           # FreeRTOS 内核库
├── cmake/                       # 工具链文件
└── release/                     # 固件发布 (不跟踪)
```

## 任务架构

| 任务 | 栈 | 频率 | 职责 |
|------|-----|------|------|
| LED | 128 | 2Hz | 500ms 翻转 PB20 |
| LCD | 1024 | 20Hz | 显示角度/距离/编码器/按键 |

## DMA 分配

| 通道 | 外设 | 用途 |
|------|------|------|
| CH0 | I2C1 RX | NCHD12 灰度读取 |
| CH1 | SPI1 TX | LCD 区域填充 + 字符渲染 |
| CH2 | I2C0 RX | JY61P 角度读取 |
| CH3 | I2C0 TX | (备用) |

## 按键功能

| 按键 | 事件 | 动作 |
|------|------|------|
| 任意 | 短按 | 编码器清零 |
| 任意 | 长按 | 预留 |

## 引脚连接

详见 `posix_demo/posix_demo.syscfg` (SysConfig GUI)。

## License

MIT
