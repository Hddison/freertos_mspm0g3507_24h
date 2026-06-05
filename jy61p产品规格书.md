# <img src="https://cdn.nlark.com/yuque/0/2024/jpeg/42882794/1718936950107-8f51ceec-deb7-476d-8274-68944d2ed661.jpeg" width="517" title="" crop="0,0,1,1" id="ube0c8336" class="ne-image">
# 产品介绍
## 产品概述
+ <font style="color:black;background-color:#FFFFFF;">该产品是基于</font>[MEMS](https://baike.baidu.com/item/MEMS)<font style="color:black;background-color:#FFFFFF;">技术的高性能三维运动姿态测量系统。它包含</font>[三轴陀螺仪](https://baike.baidu.com/item/%E4%B8%89%E8%BD%B4%E9%99%80%E8%9E%BA%E4%BB%AA/3785697)<font style="color:black;background-color:#FFFFFF;">、</font>[三轴加速度计](https://baike.baidu.com/item/%E4%B8%89%E8%BD%B4%E5%8A%A0%E9%80%9F%E5%BA%A6%E8%AE%A1/20863147)<font style="color:black;background-color:#FFFFFF;">。通过集成各种高性能传感器和运用自主研发的姿态动力学核心算法引擎，结合高动态卡尔曼滤波融合算法，为客户提供高精度、高动态、实时补偿的三轴姿态角度，通过对各类数据的灵活选择配置，满足不同的应用场景。</font>
+ <font style="background-color:#FFFFFF;">领先的基于 Kalman 滤波原理并具有自主知识产权的传感器融合算法，可以实时提供高达 200Hz 更新率的数据，从而满足各种高精度的应用需求，实现准确的动作捕捉和姿态估计。</font>
+ <font style="background-color:#FFFFFF;">拥有国内领先的高精度转台设备仪器，产品内部集成自主研发的高精度校准和标定算法，提高产品的测量精度。</font>
+ <font style="background-color:#FFFFFF;">同时提供用户所需要的各种上位机、使用说明、开发手册、开发代码，使得针对各类需求的研发时间降至最低。</font>



## 产品特点
+ <font style="color:black;background-color:#FFFFFF;">模块集成高精度的陀螺仪、加速度计，采用高性能的微处理器和先进的动力学解算与卡尔曼动态滤波算法，能够快速求解出模块当前的实时运动姿态。</font>
+ <font style="color:black;background-color:#FFFFFF;">采用先进的数字滤波技术，能有效降低测量噪声，提高测量精度。</font>
+ <font style="color:black;background-color:#FFFFFF;">模块内部集成了姿态解算器，配合动态卡尔曼滤波算法，能够在动态环境下准确输出模块的当前姿态，</font> 姿态测量精度 0.2度 <font style="color:black;background-color:#FFFFFF;">，稳定性极高，性能甚至优于某些专业的倾角仪。</font>
+ <font style="color:black;background-color:#FFFFFF;">模块内部自带电压稳定电路，工作电压3.3~5V，引脚电平兼容3.3V/5V的嵌入式系统，连接方便。</font>
+ <font style="color:black;background-color:#FFFFFF;">支持串口和IIC两种数字接口。方便用户选择最佳的连接方式。串口速率4800bps~230400bps可调，IIC接口支持全速400K速率。</font>
+ <font style="color:black;background-color:#FFFFFF;">最高200Hz数据输出速率。输出内容可以任意选择，输出速率0.2～200Hz可调节。</font>
+ <font style="color:black;background-color:#FFFFFF;">保留4路扩展端口，可以分别配置为模拟输入，数字输入，数字输出等功能。</font>
+ <font style="color:black;background-color:#FFFFFF;">采用邮票孔镀金工艺，可嵌入用户的PCB板中。</font>**<font style="color:black;background-color:#FFFFFF;"></font>**
+ <font style="color:black;background-color:#FFFFFF;">4层PCB板工艺，更薄、更小、更可靠。</font>

# <font style="color:black;background-color:#FFFFFF;">参数指标</font>
## <font style="color:black;background-color:#FFFFFF;">加速度计参数</font>
| 参数 | 条件 | 典型值 |
| :---: | --- | :---: |
| 量程 |  | ±16g |
| 分辨率 | ±16g | 0.0005(g/LSB) |
| 采样率 |  | 1KHz |
| RMS噪声 | 带宽=100Hz | 0.75~1mg-rms |
| 静止零漂 | 水平放置 | ±20~40mg |
| 温漂 | -40°C ~ +85°C | ±0.15mg/℃ |
| 带宽 |  | 5~256Hz |
| 零偏稳定性 | 10S平滑 | XY:15ug；Z:35ug |
| 零偏不稳定性 | allan方差 | XY:10ug；Z:30ug |


## 陀螺仪参数
| 参数 | 条件 | 典型值 |
| :---: | :---: | :---: |
| 量程 |  | ±2000°/s |
| 分辨率 | ±2000°/s | 0.061(°/s)/(LSB) |
| 采样率 |  | 1KHz |
| RMS噪声 | 带宽=100Hz | 0.005(°/s) |
| 静止零漂 | 水平放置 | ±0.5~1°/s |
| 温漂 | -40°C ~ +85°C | ±0.005~0.015 (°/s)/℃ |
| 带宽 |  | 5~256Hz |
| 零偏稳定性 | 10S平滑 | 8°/h |
| 零偏不稳定性 | allan方差 | 5°/h |




## 俯仰、横滚角参数
| 参数 | 条件 | 典型值 |
| :---: | :---: | :---: |
| 量程 |  | X:±180°   |
| | | Y:±90° |
| 倾角精度 |  | 0.2° |
| 分辨率 | 水平放置 | 0.0055° |
| 温漂 | -40°C ~ +85°C | ±0.2° |


## 航向角参数
| 参数 | 条件 | 典型值 |
| :---: | :---: | :---: |
| 量程 | | Z:±180° |
| 航向精度 | 6轴算法，静态 | 0.5°（动态存在积分累计误差）<br/>【1】 |
| 分辨率 | 水平放置 | 0.0055° |


注：

【1】在有些震动环境下，会有累计误差，具体误差不可估计，具体根据实际测试为准

## 模组参数
### 基本参数
| 参数 | 条件 | 最小值 | 默认 | 最大值 |
| :---: | --- | --- | --- | :---: |
| 通信接口 | UART | 4800bps | 9600bps | 230400bps |
| | 硬件I2C |  |  | 400K |
| | 模拟I2C |  |  | 100K |
| 输出内容 |  | 片上时间、3轴加速度、3轴角速度、3轴角度、四元数、端口状态 | | |
| 输出速率 |  | 0.2Hz | 10Hz | 200Hz |
| 启动时间 |  |  |  | 1000ms |
| 重量 |  |  | 1g |  |
| 工作温度 |  | -40℃ |  | 85℃ |
| <font style="color:rgb(77, 77, 77);">存储温度</font> |  | <font style="color:rgb(77, 77, 77);">-40</font>℃ | <font style="color:rgb(77, 77, 77);"></font> | <font style="color:rgb(77, 77, 77);">100</font>℃ |
| 耐冲击 |  |  |  | 20000g |


### 电气参数
| 参数 | 条件 | 最小值 | 默认 | 最大值 |
| :---: | :---: | :---: | :---: | :---: |
| 供电电压 |  | 3.3V | 5V | 5.5V |
| 工作电流 | 工作（5V） |  | 8.43mA |  |
| | 休眠（5V） |  | 9.91uA |  |
| 功耗 |  |  | 42.15mW |  |


# 产品尺寸
<img src="https://cdn.nlark.com/yuque/0/2025/png/42356723/1755571716397-c0ac6749-94b3-4f08-8296-f4a1b24eb8e7.png" width="1706" title="" crop="0,0,1,1" id="u523ddf7d" class="ne-image">

# 引脚定义及连接
## 引脚说明
<font style="color:#DF2A3F;">注： 陀螺仪传感器禁止超声波焊接、清洗、切割等操作 。</font>

| 引脚序号 | 引脚名称 | 引脚功能 |
| :---: | :---: | :---: |
| 1 | D0 | 预留端口 |
| 2 | VCC | 电源3.3~5V |
| 3 | RX | 串行数据输入 |
| 4 | TX | 串行数据输出 |
| 5 | GND | 地线 |
| 6 | D1 | 预留 |
| 7 | D3 | X+角度报警输出（报警时输出高低电平（ 通过配置这个报警里面的1就是高电平，0就是低电平  ）） |
| 8 | GND | 地线 |
| 9 | SDA | I2C数据线、Y-角度报警输出（报警时输出高低电平（ 通过配置这个报警里面的1就是高电平，0就是低电平  ）） |
| 10 | SCL | I2C时钟线、Y+角度报警输出（报警时输出高低电平（ 通过配置这个报警里面的1就是高电平，0就是低电平  ）） |
| 11 | VCC | 电源3.3~5V |
| 12 | D2 | 角度报警输出、X-角度报警输出（报警时输出高低电平（ 通过配置这个报警里面的1就是高电平，0就是低电平  ）） |


## 模块UART与MCU连接
<img src="https://cdn.nlark.com/yuque/0/2025/png/43192435/1749345905529-2219cb9e-ac1a-4e5d-98e6-4e5777df42f8.png" width="605" title="" crop="0,0,1,1" id="u1aac7f2a" class="ne-image">

## 模块I2C与MCU连接
<img src="https://cdn.nlark.com/yuque/0/2025/png/43192435/1749345909609-67e89f6c-01f5-4999-a06a-12d7a30975bc.png" width="595" title="" crop="0,0,1,1" id="uff1956e1" class="ne-image">

# 产品包装
## 样品包装
<img src="https://cdn.nlark.com/yuque/0/2022/jpeg/23054578/1648637145219-b31ac40b-fba6-462c-a73d-e7e09af7c5d3.jpeg" width="440" title="" crop="0,0,1,1" id="ua002959d" class="ne-image"><img src="https://cdn.nlark.com/yuque/0/2022/jpeg/23054578/1648637145654-268eca4a-2243-461f-9370-51fbdc01a48f.jpeg" width="286" title="" crop="0,0,1,1" id="u17d2a83e" class="ne-image">

## 批量包装
<img src="https://cdn.nlark.com/yuque/0/2022/jpeg/23054578/1648637885902-29980f25-6258-488a-a53a-d7ab6ce3f3a8.jpeg" width="541" title="" crop="0,0,1,1" id="u93938670" class="ne-image">

# <font style="color:black;background-color:#FFFFFF;">应用领域</font>
+ <font style="color:black;background-color:#FFFFFF;">虚拟现实/增强现实，头戴显示器</font>
+ <font style="color:black;background-color:#FFFFFF;">大规模农业自动耕种</font>
+ <font style="color:black;background-color:#FFFFFF;">高空作业安全监控</font>
+ <font style="color:black;background-color:#FFFFFF;">工业姿态监控</font>
+ <font style="color:black;background-color:#FFFFFF;">人体动作跟踪/捕捉</font>
+ <font style="color:black;background-color:#FFFFFF;">机器人，自动引导运输车</font>
+ <font style="color:black;background-color:#FFFFFF;">行人导航</font>
+ <font style="color:black;background-color:#FFFFFF;">无人驾驶/辅助驾驶</font>


