# MSPM0G3507 Hardware Debug Technical Notes

## Architecture
```
main.c → HW (hw_*.c) → HAL (hal_spi) → BSP (bsp_*.c)
                            ↕ FreeRTOS mutex
```

## Bus Map
| Bus | Devices | Notes |
|-----|---------|-------|
| SPI1 (10MHz) | W25Q128 Flash + ST7789 LCD | HAL mutex, CS=PB6/PB14 |
| I2C0 (1MHz) | JY61P IMU | DMA reads |
| I2C1 (100kHz) | NCHD12 Grayscale (PCA9555) | DMA reads |
| UART0 (115200) | Debug console | PA10/PA11 |
| GPIOA | Buttons (6x), Encoder (E1A/B, E2A/B) | GROUP1 ISR |
| TIMA0 | 10ms speed sampling timer | ZERO_EVENT ISR |
| TIMA1 | PWM Motor (40kHz) | PA16/PA17 |

## SPI + DMA
- SysConfig: TX FIFO threshold MUST be `1/2_EMPTY` (not `EMPTY`)
- DMA address mode: Block↔Fixed (src increment, dest fixed)
- `bsp_spi.c`: `DL_DMA_initChannel` before EVERY transfer (multi-device reuse)
- After LCD DMA TX: drain TX+RX FIFO before CS HIGH (prevents Flash contamination)
- Flash DMA TX has 3-byte hardware pipeline shift → use CPU write for Flash

## FreeRTOS Critical Config
- `configCHECK_FOR_STACK_OVERFLOW=2` → must provide `vApplicationStackOverflowHook`
- Task stacks: Flash-intensive ≥768 words, LCD ≥768 words
- `HAL_SPI_init()` before scheduler start (no lazy init race)
- ISR priority: Timer=1, Encoder(GPIOA)=2, UART=3

## Per-Module Notes

### 1. UART+LED (PB20)
- UART0 DMA TX/RX works via `bsp_uart.c`
- LED: `DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_20)` = ON

### 2. Buttons (6x, active-low)
- Raw GPIO poll at 20ms, compare prev/now for edge detection
- BSP_Button_Scan state machine had a bug → use raw poll

### 3. Buzzer (PB22, active-high)
- `Buzzer_set(1)` = ON, simple GPIO
- FreeRTOS: Task Notification trigger pattern

### 4. W25Q128 Flash (SPI1, CS=PB6)
- ID: 0xEF17
- CPU write (page program, max 256B)
- DMA RX read via `BSP_SPI_rx_dma` (internal TX dummy for clock)
- `_eraseSector` is internal no-lock version, called by `write` while holding lock

### 5. ST7789 LCD (SPI1, CS=PB14)
- 170×320, 16-bit color, DMA fill
- Font8 (5×8) for compact display
- `drawStringFast`: per-line clear+redraw for smooth update
- DMA pixel data via `BSP_SPI_tx_dma`

### 6. JY61P IMU (I2C0, 0x50)
- `JY61P_readIMU_dma`: single 12-byte DMA burst (AX~GZ contiguous)
- `JY61P_readAngle`: 6-byte DMA burst (Roll/Pitch/Yaw)
- Init: unlock → 200Hz → ACC+ANG output → calibrate → save

### 7. NCHD12 Grayscale (I2C1, PCA9555 0x20)
- DMA read 2 bytes from Input Port 0+1 → 12-bit bitmap

### 8. Motor (TIMA1 PWM) + Encoder (GPIOA ISR)
- TB6612: PWM 0-999, direction via GPIO
- Encoder: GROUP1_IRQHandler counts A/B edges
- Speed: TIMA0 10ms ISR samples `(now-prev)` delta

## itoa_simple Safety
- Use `uint32_t` for negation: `-(val+1)+1` avoids INT32_MIN overflow
- Crucial when reading volatile 32-bit encoder counts (torn reads possible)
