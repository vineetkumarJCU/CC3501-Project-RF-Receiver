## 构建一个可以在本测试工程运行的GUI库，划分好文件和各种功能的回调/接口函数，以便我后续移植到我的主工程里

## 本GUI操作界面是用于操作树莓派RP2040双核收音机的（我的个人项目）
该项目详情如下：

**RP2040**
运行在双核模式，频率192MHz，flash为W25q16

**ST7789**
SPI屏幕，320*240 pixel 设为横屏

**CST816D**
触摸屏芯片

**SI4732**
收音机芯片，使用IIC进行控制

**NS4160**
功放芯片

**GPS模块**
用于接收位置，UTC时间等等，带有使能脚

**SD卡**
用于记录位置，UTC时间戳，SI4732的接收信号强度等等

**pwm输出**
控制ST7789屏幕背光

**RP2040 's ADC ch0**
用于监测电池电量

**RP2040 's ADC ch1**
用于监测3.5mm音频插头是否插入


GUI界面要求：
划分为三个界面，以左右滑动切换：

界面1（位于中间，允许界面上下超出屏幕，允许上下滑动）：
此为收音机主界面（显示名字为`Radio dashboard`），涵盖SI4732的各项设置和信息显示，要包含的显示/控制项如下：
1. 两个互斥选项，名称为：`SW/AM/LW mode` 和 `FM mode` ，这个用于控制SI4732的工作频段

2. 频率显示窗口（显示名字为`Rx Frequency`），点击后在当前屏幕覆盖显示一个大数字键盘用于设置SI4732的接收频率（数字为0~9，带有小数点，和kHz，MHz，OK三个选项，点击OK后数字键盘消失，将当前输入的数字显示在窗口内），通过大数字键盘完成频率输入后，覆盖弹出一个窗口用于显示参数检查结果SI4732返回的状态，点击窗口内的`OK`按钮后窗口消失，窗口内的文本留空

3. 一个输入滑动条，范围为0~63，（显示名字为`AUdio volume`），这个用于控制SI4732的输出音量

4. 一个下拉框（显示名称为Channel Filter），在选择了`SW/AM/LW mode`的情况下，下拉框内容为：
`BW_6KHZ`
`BW_4KHZ`
`BW_3KHZ`
`BW_2KHZ`
`BW_1KHZ`
`BW_1K8HZ`
`BW_2K5HZ`
在选择了`FM mode`的情况下，下拉框内容为：
`BW_AUTO`
`BW_110KHZ`
`BW_84KHZ`
`BW_60KHZ`
`BW_40KHZ`

5. 一个用于显示SI4732接收状态的表（显示名称为`RX signal info`）(8行，2列)，要显示的项有如下，每个名称项对应一个文本字符串项：
`AFCRL`
`VALID`
`PILOT`
`STBLEND`
`RSSI`
`SNR`
`Multipath`
`Freq_offset`

界面2（位于左边）：
此为硬件控制界面（显示名称为`Hardware settings`），涵盖电池电压，NS4160控制，屏幕背光等等，显示项如下：
1. 一个显示框，显示当前电池电压的数值

2. 一个toggle按钮，按钮的两个状态文本为`Audio AMP ON` 和 `Audio AMP OFF` 用于控制NS4160是否启用，这个按钮也要能被软件修改状态

3. 一个下拉框（显示名称为`Audio AMP mode`）,下拉选项为：
`Class-AB`
`Class-D`

4. 一个滑动条（显示名称为`Screen backlight`），范围为0~49，用于控制pwm占空比以控制屏幕亮度


界面3（位于右边，允许界面上下超出屏幕，允许上下滑动）：
此为GPS界面，涵盖GPS坐标显示，UTC时间，SD卡log功能使能等等，显示项如下：
1. 一个开关（显示名称为`Enable GPS`），用于是否使能开启GPS

2. 一个开关（显示名称为`Record log in SD card`）,用于选择是否写入SD卡数据

3. 一个小文本框（显示名称为`SD status`）显示当前的SD卡状态，显示的状态如下：
`no SD`
`SD error`
`SD OK`

4. 一个小文本框（显示名称为`GPS status`）,显示当前的GPS状态，显示的状态如下：
`GPS locked`
`GPS unlocked`
`GPS error`

5. 一个显示当前GPS状态的表格（14行，2列）要显示的项有如下，每个名称项对应一个文本字符串项：
`Latitude`
`Longitude`
`Altitude (Sea)`
`Altitude`
`Speed`
`Direction`
`UTC time`
`UTC date`
`Magnetic declination`
`Locate mode`
`Location system`
`Location type`
`Location status`
`Satellites`

