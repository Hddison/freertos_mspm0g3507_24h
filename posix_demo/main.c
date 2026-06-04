/*
 * main.c — HW Full Self-Test (UART + LCD)
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "bsp_i2c.h"
#include "hal_spi.h"
#include "hw_buzzer.h"
#include "hw_w25q128.h"
#include "hw_st7789.h"
#include "hw_jy61p.h"
#include "hw_nchd12.h"
#include "hw_motor.h"
#include "util.h"
#include "gui_paint.h"
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_timera.h>

extern const uint8_t Font8_Table[];
#define FW 5
#define FH 10

/* ── FreeRTOS hooks ── */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    BSP_UART_tx_str("STACK: "); BSP_UART_tx_str(pcTaskName); for(;;){}
}

/* ── HardFault debug handler ── */
void HardFault_Handler(void) {
    BSP_UART_tx_str("\r\n!!! HardFault !!!\r\n");
    uint32_t ipsr; __asm volatile ("MRS %0, IPSR" : "=r" (ipsr));
    BSP_UART_tx_str("IPSR=");
    for (int s=28; s>=0; s-=4) {
        uint8_t n=(ipsr>>s)&0xF; BSP_UART_tx_byte(n<10?'0'+n:'A'+n-10);
    }
    BSP_UART_tx_str("\r\n"); for(;;){}
}

/* ── LCD helpers ── */
static void dash_line(int y, const char *label, const char *val, uint16_t color) {
    char buf[32]; int p=0;
    for (int i=0; label[i]; i++) buf[p++]=label[i];
    buf[p++]=' '; buf[p]=0;
    ST7789_setWindows(0, y, ST7789_WIDTH-1, y+FH-1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, FH);
    ST7789_drawStringFast(0, y, buf, Font8_Table, FW, 8, color, BLACK);
    ST7789_drawStringFast(50, y, val, Font8_Table, FW, 8, color, BLACK);
}

static void test_line(int row, const char *name, const char *result, uint16_t color) {
    char buf[24]; int p=0;
    buf[p++]='['; buf[p++]='0'+row; buf[p++]=']'; buf[p++]=' ';
    for (int i=0; name[i]; i++) buf[p++]=name[i];
    buf[p++]=':'; buf[p++]=' ';
    for (int i=0; result[i]; i++) buf[p++]=result[i];
    buf[p]=0;
    BSP_UART_tx_str(buf); BSP_UART_tx_str("\r\n");
    ST7789_setWindows(0, row*FH, ST7789_WIDTH-1, row*FH+FH-1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, FH);
    ST7789_drawStringFast(0, row*FH, buf, Font8_Table, FW, 8, color, BLACK);
}

#define R_PASS(name) test_line(row, name, "PASS", GREEN)
#define R_FAIL(name) test_line(row, name, "FAIL", RED)
#define R_SKIP(name) test_line(row, name, "SKIP", CYAN)

/* ── Key poll helper ── */
static int key_pressed(void) {
    GPIO_Regs *p[]={GPIO_KEY_PIN_UP_PORT,GPIO_KEY_PIN_LEFT_PORT,
        GPIO_KEY_PIN_DOWN_PORT,GPIO_KEY_PIN_RIGHT_PORT,
        GPIO_KEY_PIN_CENTER_PORT,GPIO_KEY_PIN_BUTTON_PORT};
    uint32_t m[]={GPIO_KEY_PIN_UP_PIN,GPIO_KEY_PIN_LEFT_PIN,
        GPIO_KEY_PIN_DOWN_PIN,GPIO_KEY_PIN_RIGHT_PIN,
        GPIO_KEY_PIN_CENTER_PIN,GPIO_KEY_PIN_BUTTON_PIN};
    for (int i=0; i<6; i++)
        if (DL_GPIO_readPins(p[i],m[i])==0) return i;
    return -1;
}

/* ════════════════════════════════ TESTS ════════════════════════════════ */

static int row; /* current LCD row */

/* Test 1: UART + LED */
static void test_uart_led(void) {
    test_line(row++, "UART+LED", "...", WHITE);
    BSP_UART_tx_str("[1] LED ON 500ms\r\n");
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    BSP_delay_ms(500);
    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    row--; R_PASS("UART+LED");
}

/* Test 2: Buttons */
static void test_buttons(void) {
    test_line(row++, "BUTTONS", "5s...", WHITE);
    BSP_UART_tx_str("[2] Press any key (5s timeout)\r\n");
    for (int i=0; i<250; i++) {
        int k = key_pressed();
        if (k>=0) { row--; R_PASS("BUTTONS"); return; }
        BSP_delay_ms(20);
    }
    row--; R_SKIP("BUTTONS");
}

/* Test 3: Buzzer */
static void test_buzzer(void) {
    test_line(row++, "BUZZER", "beep...", WHITE);
    Buzzer_init();
    for (int i=0; i<2; i++) { Buzzer_set(1); BSP_delay_ms(100);
                              Buzzer_set(0); BSP_delay_ms(100); }
    row--; R_PASS("BUZZER");
}

/* Test 4: Flash */
static void test_flash(void) {
    test_line(row++, "FLASH", "...", WHITE);
    DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
    BSP_delay_ms(10);
    uint16_t id = HW_W25Q128_readID();
    if (id != 0xEF17) { row--; R_FAIL("FLASH"); return; }
    uint8_t w[16], r[16];
    for (int i=0; i<16; i++) w[i]=(uint8_t)(0xA0+i);
    if (!HW_W25Q128_write(w, 0x300, 16)) { row--; R_FAIL("FLASH"); return; }
    if (!HW_W25Q128_read(r, 0x300, 16))  { row--; R_FAIL("FLASH"); return; }
    for (int i=0; i<16; i++) if (w[i]!=r[i]) { row--; R_FAIL("FLASH"); return; }
    row--; R_PASS("FLASH");
}

/* Test 5: LCD visual check */
static void test_lcd_visual(void) {
    test_line(row++, "LCD", "see scrn", WHITE);
    BSP_UART_tx_str("[5] Check LCD colors\r\n");
    for (int c=0; c<3; c++) {
        uint16_t clr[]={RED,GREEN,BLUE};
        ST7789_setWindows(0, FH*16, ST7789_WIDTH-1, FH*16+29);
        ST7789_clearRawDMA(clr[c], ST7789_WIDTH, 30);
        BSP_delay_ms(300);
    }
    ST7789_setWindows(0, FH*16, ST7789_WIDTH-1, FH*16+29);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, 30);
    row--; R_PASS("LCD");
}

/* Test 6: IMU */
static void test_imu(void) {
    test_line(row++, "IMU", "...", WHITE);
    BSP_I2C_init(); BSP_delay_ms(200);
    bool ok=false;
    for (int i=0; i<5; i++) {
        if (JY61P_init()) { ok=true; break; }
        BSP_delay_ms(500);
    }
    if (!ok) { row--; R_FAIL("IMU"); return; }
    JY61P_RawAngle ra;
    if (!JY61P_readAngle(&ra)) { row--; R_FAIL("IMU"); return; }
    row--; R_PASS("IMU");
}

/* Test 7: Grayscale */
static void test_grayscale(void) {
    test_line(row++, "GRAYSCALE", "...", WHITE);
    uint16_t bits;
    if (!NCHD12_read(&bits)) { row--; R_FAIL("GRAY"); return; }
    BSP_UART_tx_str("[7] Grayscale: 0x");
    char tmp[32];
    util_itoa(bits, tmp); BSP_UART_tx_str(tmp); BSP_UART_tx_str("\r\n");
    row--; R_PASS("GRAYSCALE");
}

/* Test 8: Motor + Encoder */
static void test_motor(void) {
    test_line(row++, "MOTOR+ENC", "...", WHITE);
    BSP_UART_tx_str("[8] Motor FWD 500ms\r\n");
    Motor_set(200, 200);
    for (int i=0; i<20; i++) {
        char tmp[40]; int p=0;
        p+=util_itoa(Motor_enc1(),tmp+p); tmp[p++]=' ';
        p+=util_itoa(Motor_enc2(),tmp+p); tmp[p++]=' ';
        p+=util_itoa(g_enc1_speed,tmp+p); tmp[p++]=' ';
        p+=util_itoa(g_enc2_speed,tmp+p);
        tmp[p]=0;
        test_line(16, "ENC", tmp, YELLOW);
        BSP_delay_ms(10);
    }
    Motor_set(0,0);
    BSP_delay_ms(500);
    if (Motor_enc1()==0 && Motor_enc2()==0) { row--; R_SKIP("MOTOR+ENC"); return; }
    row--; R_PASS("MOTOR+ENC");
}

/* ── Main Self-Test Task ── */
static void prvTestTask(void *pvParameters) {
    (void)pvParameters;
    BSP_UART_tx_str("\r\n===== HW Self-Test =====\r\n");
    row = 0;

    test_uart_led();
    test_buttons();
    test_buzzer();
    test_flash();
    test_lcd_visual();
    test_imu();
    test_grayscale();
    test_motor();

    test_line(row++, "ALL DONE", "", GREEN);
    BSP_UART_tx_str("===== Dashboard =====");

    /* 清屏准备仪表盘 */
    ST7789_setWindows(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);

    for (;;) {
        JY61P_RawAngle ra; JY61P_Angle a;
        JY61P_RawIMU ri;
        uint16_t gray;
        int32_t e1 = Motor_enc1(), e2 = Motor_enc2();
        int32_t s1 = g_enc1_speed, s2 = g_enc2_speed;

        bool imu_ok = JY61P_readAngle(&ra) && JY61P_readIMU_dma(&ri);
        bool gray_ok = NCHD12_read(&gray);

        char buf[32]; int p;

        /* --- Row 2: Angles --- */
        if (imu_ok) {
            JY61P_convAngle(&ra, &a);
            p=0; buf[p++]='R'; buf[p++]=':';
            p+=util_ftoa(a.roll,1,buf+p); buf[p++]=' ';
            buf[p++]='P'; buf[p++]=':';
            p+=util_ftoa(a.pitch,1,buf+p); buf[p++]=' ';
            buf[p++]='Y'; buf[p++]=':';
            p+=util_ftoa(a.yaw,1,buf+p);
            buf[p]=0;
        } else { buf[0]='-';buf[1]=0; }
        dash_line(FH*3, "ANGLE", buf, WHITE);

        /* --- Row 4: Gyro --- */
        if (imu_ok) {
            p=0; buf[p++]='G';buf[p++]='x';buf[p++]=':';
            p+=util_ftoa((float)(ri.gx*JY61P_GYRO_SCALE),0,buf+p); buf[p++]=' ';
            buf[p++]='G';buf[p++]='y';buf[p++]=':';
            p+=util_ftoa((float)(ri.gy*JY61P_GYRO_SCALE),0,buf+p); buf[p++]=' ';
            buf[p++]='G';buf[p++]='z';buf[p++]=':';
            p+=util_ftoa((float)(ri.gz*JY61P_GYRO_SCALE),0,buf+p);
            buf[p]=0;
        } else { buf[0]='-';buf[1]=0; }
        dash_line(FH*5, "GYRO", buf, CYAN);

        /* --- Row 6: Accel --- */
        if (imu_ok) {
            p=0; buf[p++]='A';buf[p++]='x';buf[p++]=':';
            p+=util_ftoa((float)(ri.ax*JY61P_ACC_SCALE),1,buf+p); buf[p++]=' ';
            buf[p++]='A';buf[p++]='y';buf[p++]=':';
            p+=util_ftoa((float)(ri.ay*JY61P_ACC_SCALE),1,buf+p); buf[p++]=' ';
            buf[p++]='A';buf[p++]='z';buf[p++]=':';
            p+=util_ftoa((float)(ri.az*JY61P_ACC_SCALE),1,buf+p);
            buf[p]=0;
        } else { buf[0]='-';buf[1]=0; }
        dash_line(FH*7, "ACCEL", buf, MAGENTA);

        /* --- Row 9: Grayscale --- */
        if (gray_ok) {
            util_bits12(gray, buf);
            int cnt=0; for(int i=0;i<12;i++) if(buf[i]=='1') cnt++;
            p=12; buf[p++]=' '; buf[p++]='C';buf[p++]='=';
            p+=util_itoa(cnt,buf+p);
            buf[p]=0;
        } else { buf[0]='-';buf[1]=0; }
        dash_line(FH*9,  "GRAY", buf, WHITE);

        /* --- Row 11: Encoder --- */
        p=0; buf[p++]='E';buf[p++]='1';buf[p++]=':';
        p+=util_itoa(e1,buf+p); buf[p++]=' ';
        buf[p++]='E';buf[p++]='2';buf[p++]=':';
        p+=util_itoa(e2,buf+p);
        buf[p]=0;
        dash_line(FH*11, "ENC", buf, YELLOW);

        /* --- Row 12: Speed --- */
        p=0; buf[p++]='S';buf[p++]='1';buf[p++]=':';
        p+=util_itoa(s1,buf+p); buf[p++]=' ';
        buf[p++]='S';buf[p++]='2';buf[p++]=':';
        p+=util_itoa(s2,buf+p);
        buf[p]=0;
        dash_line(FH*12, "SPD", buf, CYAN);

        
    }
}

/* ── main ── */
int main(void) {
    SYSCFG_DL_init();
    BSP_delay_ms(100);

    /* UART banner */
    BSP_UART_tx_str("\r\nMSPM0G3507 HW Self-Test\r\n");

    /* LCD init */
    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    ST7789_setWindows(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    ST7789_drawStringFast(0,0,"HW Self-Test",Font8_Table,FW,8,GREEN,BLACK);

    /* HAL init */
    HAL_SPI_init();

    /* Motor init (includes TIMA0 timer start) */
    Motor_init();
    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    /* Disable unhandled interrupts */
    NVIC_DisableIRQ(UART_0_INST_INT_IRQN);

    /* Create test task */
    if (xTaskCreate(prvTestTask, "TEST", 2048, NULL, 1, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL\r\n"); for(;;){}
    }

    BSP_UART_tx_str("Starting...\r\n");
    vTaskStartScheduler();
    for(;;){}
}
