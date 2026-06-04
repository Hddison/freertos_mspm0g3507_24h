/*
 * ============ bsp_i2c.c =============
 *
 * 所有阻塞等待均有超时保护 (I2C_TIMEOUT ≈ 500ms @ 80MHz)。
 * 超时后自动执行总线恢复, 返回 false。
 */

#include "bsp_i2c.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_i2c.h>
#include <ti/driverlib/dl_dma.h>
#include <ti/driverlib/dl_gpio.h>

#define I2C_TIMEOUT  10000000UL  /* ~500ms @ 80MHz */

/* ══════ I2C0 总线恢复 ══════ */

void BSP_I2C_recover(void)
{
    volatile uint32_t dly;

    /* 1. 重置 I2C, 将 SCL 设为 GPIO 输出, SDA 设为输入 */
    DL_I2C_reset(I2C_0_INST);
    DL_GPIO_initDigitalOutput(GPIO_I2C_0_IOMUX_SCL);
    DL_GPIO_initDigitalInputFeatures(GPIO_I2C_0_IOMUX_SDA,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearPins(GPIO_I2C_0_SCL_PORT, GPIO_I2C_0_SCL_PIN);
    DL_GPIO_enableOutput(GPIO_I2C_0_SCL_PORT, GPIO_I2C_0_SCL_PIN);

    /* 2. 发 SCL 脉冲 (最多 100 次), 直到 SDA 被释放 */
    for (int i = 0; i < 100; i++) {
        DL_GPIO_clearPins(GPIO_I2C_0_SCL_PORT, GPIO_I2C_0_SCL_PIN);
        for (dly = 80000; dly; dly--);
        DL_GPIO_setPins(GPIO_I2C_0_SCL_PORT, GPIO_I2C_0_SCL_PIN);
        for (dly = 80000; dly; dly--);
        if (DL_GPIO_readPins(GPIO_I2C_0_SDA_PORT, GPIO_I2C_0_SDA_PIN))
            break;
    }

    /* 3. 恢复 I2C 外设功能, 重新初始化 */
    DL_I2C_reset(I2C_0_INST);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_0_IOMUX_SDA,
        GPIO_I2C_0_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_0_IOMUX_SCL,
        GPIO_I2C_0_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_I2C_0_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_I2C_0_IOMUX_SCL);
    DL_I2C_enablePower(I2C_0_INST);
    SYSCFG_DL_I2C_0_init();
    DL_I2C_enableDMAEvent(I2C_0_INST, DL_I2C_EVENT_ROUTE_1,
                          DL_I2C_DMA_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    DL_I2C_enableDMAEvent(I2C_0_INST, DL_I2C_EVENT_ROUTE_2,
                          DL_I2C_DMA_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER);
}

/* ══════ I2C1 总线恢复 (通用版) ══════ */

static void i2c_recover_ex(I2C_Regs *i2c,
                           uint32_t scl_iomux, uint32_t scl_func,
                           uint32_t sda_iomux, uint32_t sda_func,
                           GPIO_Regs *scl_port, uint32_t scl_pin,
                           GPIO_Regs *sda_port, uint32_t sda_pin,
                           void (*reinit_fn)(void))
{
    volatile uint32_t dly;

    DL_I2C_reset(i2c);
    DL_GPIO_initDigitalOutput(scl_iomux);
    DL_GPIO_initDigitalInputFeatures(sda_iomux,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearPins(scl_port, scl_pin);
    DL_GPIO_enableOutput(scl_port, scl_pin);

    for (int i = 0; i < 100; i++) {
        DL_GPIO_clearPins(scl_port, scl_pin);
        for (dly = 80000; dly; dly--);
        DL_GPIO_setPins(scl_port, scl_pin);
        for (dly = 80000; dly; dly--);
        if (DL_GPIO_readPins(sda_port, sda_pin))
            break;
    }

    DL_I2C_reset(i2c);
    DL_GPIO_initPeripheralInputFunctionFeatures(sda_iomux, sda_func,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(scl_iomux, scl_func,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(sda_iomux);
    DL_GPIO_enableHiZ(scl_iomux);
    DL_I2C_enablePower(i2c);
    reinit_fn();
    DL_I2C_enableDMAEvent(i2c, DL_I2C_EVENT_ROUTE_2,
                          DL_I2C_DMA_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER);
}

/* ══════ BSP_I2C_init ══════ */

void BSP_I2C_init(void)
{
    if (DL_I2C_getSDAStatus(I2C_0_INST) == DL_I2C_CONTROLLER_SDA_LOW)
        BSP_I2C_recover();
    if (DL_I2C_getSDAStatus(I2C_NCHD12_INST) == DL_I2C_CONTROLLER_SDA_LOW)
        i2c_recover_ex(I2C_NCHD12_INST,
            GPIO_I2C_NCHD12_IOMUX_SCL, GPIO_I2C_NCHD12_IOMUX_SCL_FUNC,
            GPIO_I2C_NCHD12_IOMUX_SDA, GPIO_I2C_NCHD12_IOMUX_SDA_FUNC,
            GPIO_I2C_NCHD12_SCL_PORT, GPIO_I2C_NCHD12_SCL_PIN,
            GPIO_I2C_NCHD12_SDA_PORT, GPIO_I2C_NCHD12_SDA_PIN,
            SYSCFG_DL_I2C_NCHD12_init);
}

/* ══════ BSP_I2C_write (CPU 轮询, I2C0) ══════ */

bool BSP_I2C_write(uint8_t addr, uint8_t reg,
                   const uint8_t *data, size_t len)
{
    uint32_t tout;
    if (!len) return true;
    if (!data) return false;

    /* 发送寄存器地址 */
    DL_I2C_transmitControllerData(I2C_0_INST, reg);
    DL_I2C_clearInterruptStatus(I2C_0_INST,
        DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);

    /* 等待总线空闲 */
    tout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(I2C_0_INST)
             & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--tout == 0) { BSP_I2C_recover(); return false; }
    }

    /* 启动 TX 传输 (reg + data) */
    DL_I2C_startControllerTransfer(I2C_0_INST, addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint16_t)(len + 1));

    /* 填充 TX FIFO */
    size_t cnt = len;
    const uint8_t *ptr = data;
    tout = I2C_TIMEOUT;
    do {
        uint16_t n = DL_I2C_fillControllerTXFIFO(I2C_0_INST,
                                                  ptr, (uint16_t)cnt);
        cnt -= n; ptr += n;
        if (DL_I2C_getRawInterruptStatus(I2C_0_INST,
                DL_I2C_INTERRUPT_CONTROLLER_TX_DONE)) break;
    } while (--tout);

    return tout > 0;
}

/* ══════ BSP_I2C_read (CPU 轮询, I2C0) ══════ */

bool BSP_I2C_read(uint8_t addr, uint8_t reg,
                  uint8_t *data, size_t len)
{
    uint32_t tout;
    size_t i = 0;
    if (!len) return true;
    if (!data) return false;

    /* 发送寄存器地址, 设置读模式 */
    DL_I2C_transmitControllerData(I2C_0_INST, reg);
    I2C_0_INST->MASTER.MCTR = I2C_MCTR_RD_ON_TXEMPTY_ENABLE;
    DL_I2C_clearInterruptStatus(I2C_0_INST,
        DL_I2C_INTERRUPT_CONTROLLER_RX_DONE);

    /* 等待总线空闲 */
    tout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(I2C_0_INST)
             & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--tout == 0) { BSP_I2C_recover(); return false; }
    }

    /* 启动 RX 传输 */
    DL_I2C_startControllerTransfer(I2C_0_INST, addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, (uint16_t)len);

    /* 从 RX FIFO 读取 */
    tout = I2C_TIMEOUT;
    do {
        if (!DL_I2C_isControllerRXFIFOEmpty(I2C_0_INST)) {
            uint8_t c = DL_I2C_receiveControllerData(I2C_0_INST);
            if (i < len) data[i++] = c;
        }
    } while (!DL_I2C_getRawInterruptStatus(I2C_0_INST,
        DL_I2C_INTERRUPT_CONTROLLER_RX_DONE) && --tout);

    /* 收尾: 清空 RX FIFO */
    if (!DL_I2C_isControllerRXFIFOEmpty(I2C_0_INST)) {
        uint8_t c = DL_I2C_receiveControllerData(I2C_0_INST);
        if (i < len) data[i++] = c;
    }

    I2C_0_INST->MASTER.MCTR = 0;
    DL_I2C_flushControllerTXFIFO(I2C_0_INST);

    return (tout > 0) && (i == len);
}

/* ══════ BSP_I2C_read_dma_ex (DMA 读, 通用) ══════ */

bool BSP_I2C_read_dma_ex(I2C_Regs *i2c, uint8_t dma_ch,
                         uint8_t addr, uint8_t reg,
                         uint8_t *data, size_t len)
{
    uint32_t tout;
    if (!len) return true;
    if (!data) return false;

    /* 发送寄存器地址 */
    DL_I2C_transmitControllerData(i2c, reg);
    i2c->MASTER.MCTR = I2C_MCTR_RD_ON_TXEMPTY_ENABLE;
    DL_I2C_clearInterruptStatus(i2c,
        DL_I2C_INTERRUPT_CONTROLLER_RX_DONE);

    /* 等待总线空闲 */
    tout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(i2c)
             & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--tout == 0) goto fail;
    }

    /* 配置 DMA */
    DL_DMA_setSrcAddr(DMA, dma_ch, (uint32_t)&i2c->MASTER.MRXDATA);
    DL_DMA_setDestAddr(DMA, dma_ch, (uint32_t)data);
    DL_DMA_setTransferSize(DMA, dma_ch, (uint16_t)len);
    DL_DMA_enableChannel(DMA, dma_ch);

    /* 启动 RX 传输 */
    DL_I2C_startControllerTransfer(i2c, addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, (uint16_t)len);

    /* 等待 DMA 完成 */
    tout = I2C_TIMEOUT;
    while (DL_DMA_getTransferSize(DMA, dma_ch) != 0) {
        if (--tout == 0) { DL_DMA_disableChannel(DMA, dma_ch); goto fail; }
    }

    /* 等待总线空闲 */
    tout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(i2c)
             & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--tout == 0) { DL_DMA_disableChannel(DMA, dma_ch); goto fail; }
    }

    DL_DMA_disableChannel(DMA, dma_ch);
    i2c->MASTER.MCTR = 0;
    DL_I2C_flushControllerTXFIFO(i2c);
    return true;

fail:
    /* 总线恢复 */
    if (i2c == I2C_NCHD12_INST) {
        i2c_recover_ex(i2c,
            GPIO_I2C_NCHD12_IOMUX_SCL, GPIO_I2C_NCHD12_IOMUX_SCL_FUNC,
            GPIO_I2C_NCHD12_IOMUX_SDA, GPIO_I2C_NCHD12_IOMUX_SDA_FUNC,
            GPIO_I2C_NCHD12_SCL_PORT, GPIO_I2C_NCHD12_SCL_PIN,
            GPIO_I2C_NCHD12_SDA_PORT, GPIO_I2C_NCHD12_SDA_PIN,
            SYSCFG_DL_I2C_NCHD12_init);
    } else {
        BSP_I2C_recover();
    }
    i2c->MASTER.MCTR = 0;
    DL_I2C_flushControllerTXFIFO(i2c);
    return false;
}

/* ══════ BSP_I2C_read_dma (I2C0 快捷版) ══════ */

bool BSP_I2C_read_dma(uint8_t addr, uint8_t reg,
                      uint8_t *data, size_t len)
{
    return BSP_I2C_read_dma_ex(I2C_0_INST, DMA_I2C_RX_CHAN_ID,
                               addr, reg, data, len);
}
