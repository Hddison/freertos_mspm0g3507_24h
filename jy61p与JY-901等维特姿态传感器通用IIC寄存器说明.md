JY-901 模块可以完全通过 IIC 进行访问，IIC 通信速率最大支持 400khz，从机地址为为

7bit，默认地址为 0x50，可以通过串口指令或者 IIC 写地址的方式更改。IIC 总线上面可以

挂多个 GY-901 模块，但需提前将模块的 IIC 地址修改为不同的地址。

  模块的 IIC 协议采用寄存器地址访问的方式。每个地址内的数据均为 16 位数据，占 2

个字节

# IIC协议寄存器表格
| ADDR<br/>(Hex) | ADDR<br/>(Dec) |  REGISTER NAME   | FUNCTION |  SERIAL <br/>I/F   | Bit15 | Bit14 | Bit13 | Bit12 | Bit11 | Bit10 | Bit9 | Bit8 | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 00 | 00 | SAVE | 保存/重启/恢复出厂 | R/W | SAVE[15:0] | | | | | | | | | | | | | | | |
| 01 | 01 | CALSW | 校准模式 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | CALSW[3:0] | | | |
| 02 | 02 | RSW | 输出内容 | R/W |  |  |  |  |  | GSA | QUATER | VELOCITY | GPS | PRESS | PORT | MAG | ANGLE | GYRO | ACC | TIME |
| 03 | 03 | RRATE | 输出速率 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | RRATE[3:0] | | | |
| 04 | 04 | BAUD | 串口波特率 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | BAUD[3:0] | | | |
| 05 | 05 | AXOFFSET | 加速度X零偏 | R/W | AXOFFSET[15:0] | | | | | | | | | | | | | | | |
| 06 | 06 | AYOFFSET | 加速度Y零偏 | R/W | AYOFFSET[15:0] | | | | | | | | | | | | | | | |
| 07 | 07 | AZOFFSET | 加速度Z零偏 | R/W | AZOFFSET[15:0] | | | | | | | | | | | | | | | |
| 08 | 08 | GXOFFSET | 角速度X零偏 | R/W | GXOFFSET[15:0] | | | | | | | | | | | | | | | |
| 09 | 09 | GYOFFSET | 角速度Y零偏 | R/W | GYOFFSET[15:0] | | | | | | | | | | | | | | | |
| 0A | 10 | GZOFFSET | 角速度Z零偏 | R/W | GZOFFSET[15:0] | | | | | | | | | | | | | | | |
| 0B | 11 | HXOFFSET | 磁场X零偏 | R/W | HXOFFSET[15:0] | | | | | | | | | | | | | | | |
| 0C | 12 | HYOFFSET | 磁场Y零偏 | R/W | HYOFFSET[15:0] | | | | | | | | | | | | | | | |
| 0D | 13 | HZOFFSET | 磁场Z零偏 | R/W | HZOFFSET[15:0] | | | | | | | | | | | | | | | |
| 0E | 14 | D0MODE | D0引脚模式 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | D0MODE[3:0] | | | |
| 0F | 15 | D1MODE | D1引脚模式 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | D1MODE[3:0] | | | |
| 10 | 16 | D2MODE | D2引脚模式 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | D2MODE[3:0] | | | |
| 11 | 17 | D3MODE | D3引脚模式 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | D3MODE[3:0] | | | |
| 1A | 26 | IICADDR | 设备地址 | R/W |  |  |  |  |  |  |  |  | IICADDR[7:0] | | | | | | | |
| 1B | 27 | LEDOFF | 关闭LED灯 | R/W |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | LEDOFF |
| 1C | 28 | MAGRANGX | 磁场X校准范围 | R/W | MAGRANGX[15:0] | | | | | | | | | | | | | | | |
| 1D | 29 | MAGRANGY | 磁场Y校准范围 | R/W | MAGRANGY[15:0] | | | | | | | | | | | | | | | |
| 1E | 30 | MAGRANGZ | 磁场Z校准范围 | R/W | MAGRANGZ[15:0] | | | | | | | | | | | | | | | |
| 1F | 31 | BANDWIDTH | 带宽 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | BANDWIDTH[3:0] | | | |
| 20 | 32 | GYRORANGE | 陀螺仪量程 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | GYRORANGE[3:0] | | | |
| 21 | 33 | ACCRANGE | 加速度量程 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | ACCRANGE[3:0] | | | |
| 22 | 34 | SLEEP | 休眠 | R/W |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | SLEEP |
| 23 | 35 | ORIENT | 安装方向 | R/W |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ORIENT |
| 24 | 36 | AXIS6 | 算法 | R/W |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | AXIS6 |
| 25 | 37 | FILTK | 动态滤波 | R/W | FILTK[15:0] | | | | | | | | | | | | | | | |
| 26 | 38 | GPSBAUD | GPS波特率 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | GPSBAUD[3:0] | | | |
| 27 | 39 | READADDR | 读取寄存器 | R/W |  |  |  |  |  |  |  |  | READADDR[7:0] | | | | | | | |
| 2A | 42 | ACCFILT | 加速度滤波 | R/W | ACCFILT[15:0] | | | | | | | | | | | | | | | |
| 2D | 45 | POWONSEND | 指令启动 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | POWONSEND[3:0] | | | |
| 2E | 46 | VERSION | 版本号 | R | VERSION[15:0] | | | | | | | | | | | | | | | |
| 30 | 48 | YYMM | 年月 | R/W | MOUTH[15:8] | | | | | | | | YEAR[7:0] |
| 31 | 49 | DDHH | 日时 | R/W | HOUR[15:8] | | | | | | | | DAY[7:0] |
| 32 | 50 | MMSS | 分秒 | R/W | SECONDS[15:8] | | | | | | | | MINUTE[7:0] |
| 33 | 51 | MS | 毫秒 | R/W | MS[15:0] | | | | | | | | | | | | | | | |
| 34 | 52 | AX | 加速度X | R | AX[15:0] | | | | | | | | | | | | | | | |
| 35 | 53 | AY | 加速度Y | R | AY[15:0] | | | | | | | | | | | | | | | |
| 36 | 54 | AZ | 加速度Z | R | AZ[15:0] | | | | | | | | | | | | | | | |
| 37 | 55 | GX | 角速度X | R | GX[15:0] | | | | | | | | | | | | | | | |
| 38 | 56 | GY | 角速度Y | R | GY[15:0] | | | | | | | | | | | | | | | |
| 39 | 57 | GZ | 角速度Z | R | GZ[15:0] | | | | | | | | | | | | | | | |
| 3A | 58 | HX | 磁场X | R | HX[15:0] | | | | | | | | | | | | | | | |
| 3B | 59 | HY | 磁场Y | R | HY[15:0] | | | | | | | | | | | | | | | |
| 3C | 60 | HZ | 磁场Z | R | HZ[15:0] | | | | | | | | | | | | | | | |
| 3D | 61 | Roll | 横滚角 | R | Roll[15:0] | | | | | | | | | | | | | | | |
| 3E | 62 | Pitch | 俯仰角 | R | Pitch[15:0] | | | | | | | | | | | | | | | |
| 3F | 63 | Yaw | 航向角 | R | Yaw[15:0] | | | | | | | | | | | | | | | |
| 40 | 64 | TEMP | 温度 | R | TEMP[15:0] | | | | | | | | | | | | | | | |
| 41 | 65 | D0Status | D0引脚状态 | R | D0Status[15:0] | | | | | | | | | | | | | | | |
| 42 | 66 | D1Status | D1引脚状态 | R | D1Status[15:0] | | | | | | | | | | | | | | | |
| 43 | 67 | D2Status | D2引脚状态 | R | D2Status[15:0] | | | | | | | | | | | | | | | |
| 44 | 68 | D3Status | D3引脚状态 | R | D3Status[15:0] | | | | | | | | | | | | | | | |
| 45 | 69 | PressureL | 气压低16位 | R | PressureL[15:0] | | | | | | | | | | | | | | | |
| 46 | 70 | PressureH | 气压高16位 | R | PressureH[15:0] | | | | | | | | | | | | | | | |
| 47 | 71 | HeightL | 高度低16位 | R | HeightL[15:0] | | | | | | | | | | | | | | | |
| 48 | 72 | HeightH | 高低高16位 | R | HeightH[15:0] | | | | | | | | | | | | | | | |
| 51 | 81 | q0 | 四元数0 | R | q0[15:0] | | | | | | | | | | | | | | | |
| 52 | 82 | q1 | 四元数1 | R | q1[15:0] | | | | | | | | | | | | | | | |
| 53 | 83 | q2 | 四元数2 | R | q2[15:0] | | | | | | | | | | | | | | | |
| 54 | 84 | q3 | 四元数3 | R | q3[15:0] | | | | | | | | | | | | | | | |
| 59 | 89 | DELAYT | 报警信号延时 | R/W | DELAYT[15:0] | | | | | | | | | | | | | | | |
| 5A | 90 | XMIN | X轴角度报警最小值 | R/W | XMIN[15:0] | | | | | | | | | | | | | | | |
| 5B | 91 | XMAX | X轴角度报警最大值 | R/W | XMAX[15:0] | | | | | | | | | | | | | | | |
| 5D | 93 | ALARMPIN | 报警引脚映射 | R/W | X-ALARM[15:12] | | | | X+ALARM[11:8] | Y-ALARM[7:4] | Y+ALARM[3:0] |
| 5E | 94 | YMIN | Y轴角度报警最小值 | R/W | YMIN[15:0] | | | | | | | | | | | | | | | |
| 5F | 95 | YMAX | Y轴角度报警最大值 | R/W | YMAX[15:0] | | | | | | | | | | | | | | | |
| 61 | 97 | GYROCALITHR | 陀螺仪静止阈值 | R/W | GYROCALITHR[15:0] | | | | | | | | | | | | | | | |
| 62 | 98 | ALARMLEVEL | 角度报警电平 | R/W |  |  |  |  |  |  |  |  |  |  |  |  | ALARMLEVEL[3:0] | | | |
| 63 | 99 | GYROCALTIME | 陀螺仪自动校准时间 | R/W | GYROCALTIME[15:0] | | | | | | | | | | | | | | | |
| 68 | 104 | TRIGTIME | 报警连续触发时间 | R/W | TRIGTIME[15:0] | | | | | | | | | | | | | | | |
| 69 | 105 | KEY | 解锁 | R/W | KEY[15:0] | | | | | | | | | | | | | | | |
| 6A | 106 | WERROR | 陀螺仪变化值 | R | WERROR[15:0] | | | | | | | | | | | | | | | |
| 6E | 110 | WZTIME | 角速度连续静止时间 | R/W | WZTIME[15:0] | | | | | | | | | | | | | | | |
| 6F | 111 | WZSTATIC | 角速度积分阈值 | R/W | WZSTATIC[15:0] | | | | | | | | | | | | | | | |
| 79 | 121 | XREFROLL | 横滚角零位参考值 | R | XREFROLL[15:0] | | | | | | | | | | | | | | | |
| 7A | 122 | YREFPITCH | 俯仰角零位参考值 | R | YREFPITCH[15:0] | | | | | | | | | | | | | | | |
| 7F | 127 | NUMBERID1 | 设备编号1-2 | R | ID2[15:8] | | | | | | | | ID1[7:0] |
| 80 | 128 | NUMBERID2 | 设备编号3-4 | R | ID4[15:8] | | | | | | | | ID3[7:0] |
| 81 | 129 | NUMBERID3 | 设备编号5-6 | R | ID6[15:8] | | | | | | | | ID5[7:0] |
| 82 | 130 | NUMBERID4 | 设备编号7-8 | R | ID8[15:8] | | | | | | | | ID7[7:0] |
| 83 | 131 | NUMBERID5 | 设备编号9-10 | R | ID10[15:8] | | | | | | | | ID9[7:0] |
| 84 | 132 | NUMBERID6 | 设备编号11-12 | R | ID12[15:8] | | | | | | | | ID11[7:0] |








### KEY（解锁）
| 寄存器名称: KEY<br/>寄存器地址: 105 (0x69)<br/>读写方向: R/W<br/>默认值: 0x0000 | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | KEY[15:0] | 解锁寄存器：进行写操作时，需要先设置该寄存器 |
| 示例：解锁，往该寄存器写0xB588（其他值无效） | | |


### SAVE（保存/重启/恢复出厂）
| 寄存器名称: SAVE<br/>寄存器地址: 0 (0x00)<br/>读写方向: R/W<br/>默认值: 0x00  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | SAVE[15:0] | 保存: 0x00<br/>重启: 0xFF<br/>恢复出厂: 0x01 |


### CALSW（校准模式）
| 寄存器名称: CALSW<br/>寄存器地址: 1 (0x01)<br/>读写方向: R/W<br/>默认值: 0x00 | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:4 |  |  |
| 3:0 | CAL[3:0] | 设置校准模式： <br/>0000(0x00): 正常工作模式<br/>0001(0x01): 自动加计校准<br/>0011(0x03): 高度清零<br/>0100(0x04): 航向角置零<br/>0111(0x07): 磁场校准（球型拟合法）<br/>1000(0x08): 设置角度参考<br/>1001(0x09): 磁场校准（双平面模式） |


### RSW（输出内容）
**<font style="color:#DF2A3F;">指令需要计算，计算教程请查看如下视频</font>**

[设置输出内容解析.mp4](https://wit-motion.yuque.com/attachments/yuque/0/2025/mp4/32619495/1755080590026-8926e28f-d462-4006-b6e6-1871aaa5a93e.mp4)

| 寄存器名称: RSW<br/>寄存器地址: 2 (0x02)<br/>读写方向: R/W<br/>默认值: 0x1E | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:11 |  |  |
| 10 | GSA (0x5A) | 0: 关闭          1: 打开 |
| 9 | QUATER (0x59) | 0: 关闭          1: 打开 |
| 8 | VELOCITY (0x58) | 0: 关闭          1: 打开 |
| 7 | GPS (0x57) | 0: 关闭          1: 打开 |
| 6 | PRESS (0x56) | 0: 关闭          1: 打开 |
| 5 | PORT (0x55) | 0: 关闭          1: 打开 |
| 4 | MAG (0x54) | 0: 关闭          1: 打开 |
| 3 | ANGLE (0x53) | 0: 关闭          1: 打开 |
| 2 | GYRO (0x52) | 0: 关闭          1: 打开 |
| 1 | ACC (0x51) | 0: 关闭          1: 打开 |
| 0 | TIME (0x50) | 0: 关闭          1: 打开 |


### RRATE（输出速率）
| 寄存器名称: RRATE<br/>寄存器地址: 3 (0x03)<br/>读写方向: R/W<br/>默认值: 0x06  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:4 |  |  |
| 3:0 | RRATE[3:0] | 设置输出速率：<br/>0001(0x01): 0.2Hz<br/>0010(0x02): 0.5Hz<br/>0011(0x03): 1Hz<br/>0100(0x04): 2Hz<br/>0101(0x05): 5Hz<br/>0110(0x06): 10Hz<br/>0111(0x07): 20Hz<br/>1000(0x08): 50Hz<br/>1001(0x09): 100Hz<br/>1011(0x0B): 200Hz<br/>1100(0x0C): 单次回传 |


### BAUD（串口波特率）
| 寄存器名称: BAUD<br/>寄存器地址: 4 (0x04)<br/>读写方向: R/W<br/>默认值: 0x02  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:4 |  |  |
| 3:0 | BAUD[3:0] | 设置串口波特率： <br/>0001(0x01): 4800bps<br/>0010(0x02): 9600bps<br/>0011(0x03): 19200bps<br/>0100(0x04): 38400bps<br/>0101(0x05): 57600bps<br/>0110(0x06): 115200bps<br/>0111(0x07): 230400bps<br/>1000(0x08): 460800bps（仅WT931/JY931/HWT606/HWT906支持）<br/>1001(0x09): 921600bps（仅WT931/JY931/HWT606/HWT906支持） |


### 
### D0MODE~D3MODE（端口模式设置）
| 寄存器名称: D0MODE~D3MODE<br/>寄存器地址: 14~17 (0x0E~0x11)<br/>读写方向: R/W  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 3:0 | D0MODE[3:0] | <font style="color:rgb(24, 24, 24);">设置D0端口模式</font><br/><font style="color:rgb(24, 24, 24);">0000(0x00)</font>: <font style="color:rgb(24, 24, 24);">模拟输入（默认）</font><br/><font style="color:rgb(24, 24, 24);">0001(0x01)</font>: <font style="color:rgb(24, 24, 24);">数字输入</font><br/><font style="color:rgb(24, 24, 24);">0010(0x02)</font>: <font style="color:rgb(24, 24, 24);">输出数字高电平</font><br/><font style="color:rgb(24, 24, 24);">0011(0x03)</font>: <font style="color:rgb(24, 24, 24);">输出数字低电平</font> |
| 3:0 | D1MODE[3:0] | <font style="color:rgb(24, 24, 24);">设置D1端口模式</font><br/><font style="color:rgb(24, 24, 24);">0000(0x00)</font>: <font style="color:rgb(24, 24, 24);">模拟输入（默认）</font><br/><font style="color:rgb(24, 24, 24);">0001(0x01)</font>: <font style="color:rgb(24, 24, 24);">数字输入</font><br/><font style="color:rgb(24, 24, 24);">0010(0x02)</font>: <font style="color:rgb(24, 24, 24);">输出数字高电平</font><br/><font style="color:rgb(24, 24, 24);">0011(0x03)</font>: <font style="color:rgb(24, 24, 24);">输出数字低电平</font><br/><font style="color:rgb(24, 24, 24);">0101(0x05)</font>: <font style="color:rgb(24, 24, 24);">设置相对姿态</font> |
| 3:0 | D2MODE[3:0] | <font style="color:rgb(24, 24, 24);">设置D2端口模式</font><br/><font style="color:rgb(24, 24, 24);">0000(0x00)</font>: <font style="color:rgb(24, 24, 24);">模拟输入（默认）</font><br/><font style="color:rgb(24, 24, 24);">0001(0x01)</font>: <font style="color:rgb(24, 24, 24);">数字输入</font><br/><font style="color:rgb(24, 24, 24);">0010(0x02)</font>: <font style="color:rgb(24, 24, 24);">输出数字高电平</font><br/><font style="color:rgb(24, 24, 24);">0011(0x03)</font>: <font style="color:rgb(24, 24, 24);">输出数字低电平</font> |
| 3:0 | D3MODE[3:0] | <font style="color:rgb(24, 24, 24);">设置D3端口模式</font><br/><font style="color:rgb(24, 24, 24);">0000(0x00)</font>: <font style="color:rgb(24, 24, 24);">模拟输入（默认）</font><br/><font style="color:rgb(24, 24, 24);">0001(0x01)</font>: <font style="color:rgb(24, 24, 24);">数字输入</font><br/><font style="color:rgb(24, 24, 24);">0010(0x02)</font>: <font style="color:rgb(24, 24, 24);">输出数字高电平</font><br/><font style="color:rgb(24, 24, 24);">0011(0x03)</font>: <font style="color:rgb(24, 24, 24);">输出数字低电平</font> |


### IICADDR（设备地址）
| 寄存器名称: IICADDR<br/>寄存器地址: 26 (0x1A)<br/>读写方向: R/W<br/>默认值: 0x50  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:8 |  |  |
| 7:0 | IICADDR[7:0] | <font style="color:rgb(24, 24, 24);">设置设备地址，用于I2C和Modbus通讯使用</font><br/><font style="color:rgb(24, 24, 24);">0x01~0x7F</font> |






### BANDWIDTH（带宽）
| 寄存器名称: BANDWIDTH<br/>寄存器地址: 31 (0x1F)<br/>读写方向: R/W<br/>默认值: 0x04  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:4 |  |  |
| 3:0 | BANDWIDTH[3:0] | <font style="color:rgb(24, 24, 24);">设置带宽</font><br/><font style="color:rgb(24, 24, 24);">0000(0x00)</font>: <font style="color:rgb(24, 24, 24);">256Hz</font><br/><font style="color:rgb(24, 24, 24);">0001(0x01)</font>: <font style="color:rgb(24, 24, 24);">188Hz</font><br/><font style="color:rgb(24, 24, 24);">0010(0x02)</font>: <font style="color:rgb(24, 24, 24);">98Hz</font><br/><font style="color:rgb(24, 24, 24);">0011(0x03)</font>: <font style="color:rgb(24, 24, 24);">42Hz</font><br/><font style="color:rgb(24, 24, 24);">0100(0x04)</font>: <font style="color:rgb(24, 24, 24);">20Hz</font><br/><font style="color:rgb(24, 24, 24);">0101(0x05)</font>: <font style="color:rgb(24, 24, 24);">10Hz</font><br/><font style="color:rgb(24, 24, 24);">0110(0x06)</font>: <font style="color:rgb(24, 24, 24);">5Hz</font> |


### 
### ORIENT（安装方向）
| 寄存器名称: ORIENT<br/>寄存器地址: 35 (0x23)<br/>读写方向: R/W | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:1 |  |  |
| 0 | ORIENT | <font style="color:rgb(24, 24, 24);">设置安装方向</font><br/><font style="color:rgb(24, 24, 24);">0(0x00)</font>: <font style="color:rgb(24, 24, 24);">水平安装</font><br/><font style="color:rgb(24, 24, 24);">1(0x01)</font>: <font style="color:rgb(24, 24, 24);">垂直安装（必须坐标轴的Y轴箭头朝上）</font> |


### AXIS6（算法）
| 寄存器名称: AXIS6<br/>寄存器地址: 36 (0x24)<br/>读写方向: R/W | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:1 |  |  |
| 0 | AXIS6 | <font style="color:rgb(24, 24, 24);">设置算法</font><br/><font style="color:rgb(24, 24, 24);">0(0x00)：9轴算法（磁场解算航行角，绝对航向角）</font><br/><font style="color:rgb(24, 24, 24);">1(0x01)：6轴算法（积分解算航行角，相对航向角）</font> |


### FILTK（K值滤波）
| 寄存器名称: FILTK<br/>寄存器地址: 37 (0x25)<br/>读写方向: R/W<br/>默认值: 0x1E | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | FILTK[15:0] | 范围：1~10000，默认30（不建议修改，一旦修改，角度达不到使用要求时，请修改为30）<br/>FILTK[15:0]越小，越相信角速度的数据，抗震性能增强，实时性减弱，<br/>全部取角速度，则设置为1。<br/>FILTK[15:0]越大，越相信加速度的数据，抗震性能减弱，实时性增强，<br/>全部取加速度，则设置为10000。 |




### READADDR（读取寄存器）
| 寄存器名称: READADDR<br/>寄存器地址: 39 (0x27)<br/>读写方向: R/W | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:8 |  |  |
| 7:0 | READADDR[7:0] | 读取寄存器范围： 请参考“寄存器表” |


### ACCFILT（加速度滤波）
| 寄存器名称: ACCFILT<br/>寄存器地址: 42 (0x2A)<br/>读写方向: R/W<br/>默认值: 0x01F4 | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | ACCFILT[15:0] | 范围：1~10000，默认500（不建议修改，一旦修改，角度达不到使用要求时，请修改为500）<br/>ACCFILT[15:0]越小，抗震性能增强，实时性减弱<br/>ACCFILT[15:0]越大，抗震性能减弱，实时性增强<br/>该参数为经验值，需要根据不同环境调试该参数，在拖拉机的环境里，<br/>ACCFILT[15:0]可调节为100，因为拖拉机的抖动严重，需要提高抗震性能 |


### 
### VERSION（版本号）
| 寄存器名称: VERSION<br/>寄存器地址: 46 (0x2E)<br/>读写方向: R<br/>默认值: 无 | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | VERSION[15:0] | 不同产品，版本号不一样 |


### YYMM~MS（片上时间）
| 寄存器名称: YYMM~MS<br/>寄存器地址: 48~51 (0x30~0x33)<br/>读写方向: R/W<br/>默认值: 0x0000  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:8 | YYMM[15:8] | 月 |
| 7:0 | YYMM[7:0] | 年 |
| 15:8 | DDHH[15:8] | 时 |
| 7:0 | DDHH[7:0] | 日 |
| 15:8 | MMSS[15:8] | 秒 |
| 7:0 | MMSS[7:0] | 分 |
| 15:0 | MS[15:0] | 毫秒 |


### AX~AZ（加速度）
| 寄存器名称: AX~AZ<br/>寄存器地址: 52~54 (0x34~0x36)<br/>读写方向: R | | |
| --- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | AX[15:0] | 加速度X=AX[15:0]/32768*16g (g为重力加速度，可取9.8m/s2) |
| 15:0 | AY[15:0] | 加速度Y=AY[15:0]/32768*16g (g为重力加速度，可取9.8m/s2) |
| 15:0 | AZ[15:0] | 加速度Z=AZ[15:0]/32768*16g (g为重力加速度，可取9.8m/s2) |
| <br/> | | |
| | | |


### GX~GZ（角速度）
| 寄存器名称: GX~GZ<br/>寄存器地址: 55~57 (0x37~0x39)<br/>读写方向: R<br/>默认值: 0x0000  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | GX[15:0] | 角速度X=GX[15:0]/32768*2000°/s |
| 15:0 | GY[15:0] | 角速度Y=GY[15:0]/32768*2000°/s |
| 15:0 | GZ[15:0] | 角速度Z=GZ[15:0]/32768*2000°/s |


### HX~HZ（磁场）
| 寄存器名称: HX~HZ<br/>寄存器地址: 58~60 (0x3A~0x3C)<br/>读写方向: R | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | HX[15:0] | 磁场X=HX[15:0] (单位：LSB) |
| 15:0 | HY[15:0] | 磁场Y=HY[15:0] (单位：LSB) |
| 15:0 | HZ[15:0] | 磁场Z=HZ[15:0] (单位：LSB) |


### Roll~Yaw（角度）
| 寄存器名称: Roll~Yaw<br/>寄存器地址: 61~63 (0x3D~0x3F)<br/>读写方向: R<br/>默认值: 0x0000  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | Roll[15:0] | 滚转角X=Roll[15:0]/32768*180° |
| 15:0 | Pitch[15:0] | 俯仰角Y=Pitch[15:0]/32768*180° |
| 15:0 | Yaw[15:0] | 航向角Z=Yaw[15:0]/32768*180° |


### TEMP（温度）
| 寄存器名称: TEMP<br/>寄存器地址: 64 (0x40)<br/>读写方向: R<br/>默认值: 0x0000  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | TEMP[15:0] | 温度=TEMP[15:0]/100℃ |


### D0Status~D3Status（端口状态）
| 寄存器名称: D0Status~D3Status<br/>寄存器地址: 65~68 (0x41~0x44)<br/>读写方向: R<br/>默认值: 0x0000 | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | D0Status[15:0] | D0状态值 |
| 15:0 | D1Status[15:0] | D1状态值 |
| 15:0 | D2Status[15:0] | D2状态值 |
| 15:0 | D3Status[15:0] | D3状态值 |


### PressureL~HeightH（气压高度）
| 寄存器名称: PressureL~HeightH<br/>寄存器地址: 69~72 (0x45~0x48)<br/>读写方向: R<br/>默认值: 0x0000  | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | PressureL[15:0] | 气压=((int)PressureH[15:0]<<16)|PressureL[15:0]<font style="color:rgb(24, 24, 24);">(Pa)</font> |
| 15:0 | PressureH[15:0] | |
| 15:0 | HeightL[15:0] | 高度=((int)HeightH[15:0]<<16)|HeightL[15:0]<font style="color:rgb(24, 24, 24);">(cm)</font> |
| 15:0 | HeightH[15:0] | |


### 
### q0~q3（四元数）
| 寄存器名称: q0~q3<br/>寄存器地址: 81~84 (0x51~0x54)<br/>读写方向: R | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | q0[15:0] | <font style="color:rgb(24, 24, 24);">四元数0=q0[15:0]/32768</font> |
| 15:0 | q1[15:0] | <font style="color:rgb(24, 24, 24);">四元数1=q1[15:0]/32768</font> |
| 15:0 | q2[15:0] | <font style="color:rgb(24, 24, 24);">四元数2=q2[15:0]/32768</font> |
| 15:0 | q3[15:0] | <font style="color:rgb(24, 24, 24);">四元数3=q3[15:0]/32768</font> |




### GYROCALTIME（陀螺仪自动校准时间）
| 寄存器名称: GYROCALTIME<br/>寄存器地址: 99 (0x63)<br/>读写方向: R/W<br/>默认值: 0x03E8 | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | GYROCALTIME[15:0] | 设置陀螺仪自动校准时间 |
| 示例：设置陀螺仪自动校准时间500ms<br/> 63 F4 01<br/>当角速度变化小于"GYROCALITHR"时，且持续500ms的时间，传感器识别为静止，自动把小于0.05°/s的角速度归零<br/>该寄存器需要需要结合GYROCALITHR寄存器使用 | | |






### NUMBERID1~NUMBERID6（设备编号）
| 寄存器名称: NUMBERID1~NUMBERID6<br/>寄存器地址: 127~132 (0x7F~0x84)<br/>读写方向: R<br/>默认值: 无 | | |
| --- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 15:0 | NUMBERID1[15:0] |  |
| 15:0 | NUMBERID2[15:0] |  |
| 15:0 | NUMBERID3[15:0] |  |
| 15:0 | NUMBERID4[15:0] |  |
| 15:0 | NUMBERID5[15:0] |  |
| 15:0 | NUMBERID6[15:0] |  |
| 设备标号：WT4200000001 | | |






### D0MS（d0报警输出时间）
<font style="color:#DF2A3F;">仅支持JY901L</font>

 DOMSL:时间的低八位，单位ms。 

DOMSH:时间的高八位，单位ms。 

模块报警输出时间默认是500ms，范围1ms-32000ms（报警电平为高电平）。

 

| 寄存器名称: <br/>寄存器地址: 143 (0x8F)<br/>读写方向: R/W | | |
| :--- | --- | --- |
| Bit |  NAME   |  FUNCTION   |
| 0 | D0MS | 1~32000ms |
|  例:报警时间要设置1000ms，1000的十六进制是0x3E8,则是 FF AA 8F E8 03   | | |




### ROUSE（震动唤醒阈值）
<font style="color:#DF2A3F;">仅支持JY901L</font>

| | 寄存器名称: <br/>寄存器地址: 144 (0x90)<br/>读写方向: R/W | | |
| --- | :--- | --- | --- |
| | Bit |  NAME   |  FUNCTION   |
| | 15:1 |  |  |
| | 0 | ROUSE | <font style="color:#DF2A3F;">ROUSE:震动唤醒寄存器设置范围0~255（对应加速度0~1G）  </font> |
| | 示例：FF AA 90 00 00（设置0） | | |






# 注意事项
1、嵌入电路板；2、短距离（理论可以1米，建议10厘米以内）3、超过10厘米接线方式的一律不支持IIC，推荐串口方式；4、一问一答方式，同485的Modbus方式，不主动输出；5、支持多连接，同WT901C485产品，连接多个需要更改ID6、IIC电路设计，SDA和SCL脚必须上拉电阻，大部分为4.7K和10K，特殊电路除外   硬件IIC开漏输出7、接线方式同485，SCL接SCL，SDA接SDA；

# iIC接线图
<img src="https://cdn.nlark.com/yuque/0/2022/png/26315485/1669603873555-0ee0c665-39c1-4f67-b10d-44d0619f5cf0.png" width="666.6666666666666" title="" crop="0,0,1,1" id="u8cd26e45" class="ne-image">

# IIC通讯的时序图参考标准：
## 1 、IIC 写入
IIC 写入的时序数据格式如下

IICAddr<<1 RegAddr Data1L Data1H Data2L Data2H ……

首先 IIC 主机向 JY-901L 模块发送一个 Start 信号，在将模块的 IIC 地址 IICAddr 写

入，在写入寄存器地址 RegAddr，在顺序写入第一个数据的低字节，第一个数据的高字

节，如果还有数据，可以继续按照先低字节后高字节的顺序写入，当最后一个数据写完

以后，主机向模块发送一个停止信号，让出 IIC 总线。

当高字节数据传入 JY-901L 模块以后，模块内部的寄存器将更新并执行相应的指令，

同时模块内部的寄存器地址自动加 1，地址指针指向下一个需要写入的寄存器地址，这

样可以实现连续写入。

以设置端口 0 为高电平输出模式为例，RegAddr 为 0x0e，DataL 为 0x02，DataH 为

0x00。逻辑分析仪捕获的波形如下图所示：<img src="https://cdn.nlark.com/yuque/0/2024/png/32619495/1715216905644-ac7a5bad-ff05-4869-8bfa-1908fad049bf.png" width="1271.6308065452813" title="" crop="0,0,1,1" id="u5b3b164d" class="ne-image">



## 2、 IIC 读取
IIC 写入的时序数据格式如下

IICAddr<<1 RegAddr (IICAddr<<1)|1 Data1L Data1H Data2L Data2H ……

首先 IIC 主机向 JY-901L 模块发送一个 Start 信号，在将模块的 IIC 地址 IICAddr 写

入，在写入寄存器地址 RegAddr，主机再向模块发送一个读信号(IICAddr<<1)|1，如果是

默认地址 0x50，那么发送的数据为 0xa1，此后模块将按照先低字节，后高字节的顺序输

出数据，主机需在收到每一个字节后，拉低 SDA 总线，向模块发出一个应答信号，待

接收完指定数量的数据以后，主机不再向模块回馈应答信号，此后模块将不再输出数据，

主机向模块再发送一个停止信号，以结束本次操作。

以读出模块的角度数据为例，RedAddr 为 0x3d、0x3e、0x3f，连续读取 6 个字节，逻

辑分析仪捕获的波形如下图所示：

<img src="https://cdn.nlark.com/yuque/0/2024/png/32619495/1715217025414-6caf49c9-8b16-4f6e-baf9-fcdd1ef746a5.png" width="1344.000039438102" title="" crop="0,0,1,1" id="u65b65826" class="ne-image">

从 0x3d 开始读取出来的数据依次为 0x9C,0x82,0x28,0xFF,0xE6,0x24。也就是说 X 轴的角度

为 0x829C，Y 轴的角度为 0xFF28，Z 轴的角度为 0x24E6。按照 7.2.4 节的公式可以求出转

化出来的角度为：X 轴角度-176.33°，Y 轴角度为-1.19°，Z 轴角度为 51.89°。

##  3.IIC 示例参考读写示例程序
[https://wiki.lckfb.com/zh-hans/dmx/module/sensor/jy61p-measurement-sensor.html](https://wiki.lckfb.com/zh-hans/dmx/module/sensor/jy61p-measurement-sensor.html)



STM32F1示例代码下载：[STM32Core_SDK_IIC.rar](https://wit-motion.yuque.com/attachments/yuque/0/2025/rar/59816103/1761201371739-9e9eac1c-be3e-40c0-b3c8-786bb94a55e6.rar)





# IIC通讯设置指令发送的流程


**写入示例步骤**

<img src="https://cdn.nlark.com/yuque/0/2025/png/32619495/1755080385631-3b448342-ce5f-4357-bb03-611eb5617002.png" width="762.0923300550118" title="" crop="0,0,1,1" id="u64f69ccc" class="ne-image">

<img src="https://cdn.nlark.com/yuque/0/2025/png/32619495/1755080395947-5aac3c07-e273-4606-8800-a77130a82b3e.png" width="796.0615618210297" title="" crop="0,0,1,1" id="ucbfb3fc9" class="ne-image">

### 

