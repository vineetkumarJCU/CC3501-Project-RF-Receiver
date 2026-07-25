# RP2040 Radio GUI Framework — LVGL v9.5.0

这是一个面向 320×240 横屏收音机的 LVGL GUI 框架，同时提供 Windows 11 x64 + SDL2 测试程序。

- LVGL：`v9.5.0`，完整源码位于 `lvgl/`
- SDL2：`release-2.32.10`，完整源码位于 `SDL/`
- 桌面构建：CMake + Ninja + MinGW-w64 x86_64
- 目标页面：Hardware / Radio / RX signal waveform / GPS，左右滑动切换
- 默认页面：中间的 `Radio dashboard`
- UI 与硬件解耦：操作通过 callback 输出，状态通过 setter API 输入

## 构建和运行

```powershell
cmake --preset mingw64-release
cmake --build --preset mingw64-release
.\build\bin\rp2040_radio_gui_demo.exe
```

鼠标拖动可模拟 CST816D 的左右/上下滑动，鼠标滚轮可滚动 Radio 与 GPS 页面。可设置环境变量 `RADIO_GUI_START_PAGE=waveform` 直接打开波形页。

## 源码划分

```text
src/
├── gui/
│   ├── radio_gui.h              # 可移植公共 API
│   ├── radio_gui_internal.h     # 内部对象与视觉定义
│   ├── radio_gui.c              # 框架、公共样式、键盘和结果弹窗
│   ├── radio_gui_radio.c        # SI4732 收音机页面
│   ├── radio_gui_waveform.c     # 256 点时域波形和对数幅度频谱页面
│   ├── radio_gui_hardware.c     # 电池、功放、背光页面
│   └── radio_gui_gps.c          # GPS、SD 日志和定位数据页面
├── demo/
│   ├── radio_gui_demo_backend.h
│   └── radio_gui_demo_backend.c # 仅用于 Windows 的模拟数据/回调
└── main.c                       # SDL 测试入口
```

移植到主工程时只需要 `src/gui/`；`src/demo/` 和 SDL 入口不需要移植。

## 硬件操作回调

在 `radio_gui_config_t` 中连接主工程逻辑：

- `band_changed`：SW/AM/LW 与 FM 模式
- `frequency_submitted`：接收频率、单位和当前频段，并返回参数检查结果
- `volume_changed`：SI4732 音量 `0..63`
- `channel_filter_changed`：当前频段与滤波器名称
- `audio_amp_enabled_changed`：NS4160 使能
- `audio_amp_mode_changed`：Class-AB / Class-D
- `backlight_changed`：PWM 背光 `0..49`
- `gps_enabled_changed`：GPS 模块使能
- `sd_logging_changed`：SD 卡记录使能

示例：

```c
static void volume_changed(void *context, uint8_t volume)
{
    radio_app_t *app = context;
    app->requested_volume = volume;
    /* 通知硬件任务，不要在 LVGL 回调里长时间阻塞。 */
}

radio_gui_config_t config = {
    .callbacks.volume_changed = volume_changed,
    .callback_context = &radio_app,
};

radio_gui_t *gui = radio_gui_create(lv_screen_active(), &config);
```

## 软件更新 GUI

驱动或应用任务取得新数据后，在 LVGL 所在线程/任务中调用 setter：

```c
radio_gui_set_battery_voltage(gui, 3.97f);
radio_gui_set_audio_amp_enabled(gui, true);
radio_gui_set_frequency_text(gui, "101.70 MHz");
radio_gui_set_rx_field(gui, RADIO_GUI_RX_RSSI, "48 dBuV");
radio_gui_set_time_domain_data(gui, time_samples, 8000U);
radio_gui_set_spectrum_data(gui, spectrum_db, 24000U, -120, 0);
radio_gui_set_sd_status(gui, RADIO_GUI_SD_OK);
radio_gui_set_gps_status(gui, RADIO_GUI_GPS_LOCKED);
radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LATITUDE, "27.4698 S");
```

这些 setter 不会反向触发硬件 callback，因此可安全用于软件同步开关和显示状态。

### 波形数据接口

两个图均固定为 `RADIO_GUI_WAVEFORM_POINT_COUNT`（256）点，并在 setter 内立即复制数据，因此调用返回后可复用输入缓冲区：

```c
int8_t time_samples[RADIO_GUI_WAVEFORM_POINT_COUNT];
int8_t spectrum_db[RADIO_GUI_WAVEFORM_POINT_COUNT];

/* 时域：有符号 8 bit 定点数；8000 Hz 用于生成真实的 ms 横轴刻度。 */
radio_gui_set_time_domain_data(gui, time_samples, 8000U);

/* 频谱：1 LSB = 1 dB；末点对应 24 kHz；纵轴显示 -120..0 dB。 */
radio_gui_set_spectrum_data(gui, spectrum_db, 24000U, -120, 0);
```

将采样率或最大频率传入 `0` 时，横轴分别回退为采样点序号或 FFT bin 序号。和其他 LVGL API 一样，这两个 setter 必须由 LVGL 所属任务调用。

## 读取当前 GUI 状态

所有可设置状态均提供对应 getter。getter 不会触发 callback，且应与其他 LVGL API 一样在 LVGL 所属任务中调用：

```c
radio_gui_band_t band = radio_gui_get_band(gui);
uint8_t volume = radio_gui_get_volume(gui);
bool amp_enabled = radio_gui_get_audio_amp_enabled(gui);
radio_gui_amp_mode_t amp_mode = radio_gui_get_audio_amp_mode(gui);
uint8_t backlight = radio_gui_get_backlight(gui);
bool gps_enabled = radio_gui_get_gps_enabled(gui);
bool sd_logging = radio_gui_get_sd_logging_enabled(gui);

char filter_name[32];
if(radio_gui_get_channel_filter(gui, filter_name, sizeof(filter_name))) {
    /* 使用当前 Channel Filter。 */
}
```

当前页面、频率文本/单位、电池电压、RX 表字段、SD/GPS 状态和 GPS 表字段也有相应的 `radio_gui_get_*()` 接口。字符串 getter 返回的指针由 LVGL 持有，在控件文本被更新或 GUI 被销毁后失效。

## 频率输入流程

1. 点击 `Rx Frequency` 主卡片。
2. 使用全屏数字键盘输入数字/小数点并选择 `kHz` 或 `MHz`。
3. 点击 `OK` 后调用 `frequency_submitted`。
4. 回调结果显示在状态弹窗；点击弹窗 `OK` 后清空文本并关闭。

如果 SI4732 检查是异步完成的，也可以稍后调用：

```c
radio_gui_show_tune_result(gui, true, "SI4732 tune completed");
```

## RP2040 集成注意事项

- GUI 框架不直接包含 SI4732、CST816D、ST7789、GPS、SD、ADC 或 PWM 驱动头文件。
- 保持所有 LVGL API 在同一个 LVGL 任务中调用；其他核心/任务应通过队列或通知传递数据。
- CST816D 的触摸点和手势应由显示移植层转换为 LVGL pointer 输入设备。
- 桌面测试使用 32 位颜色；移植 ST7789 时可在 `lv_conf.h` 调整为目标工程实际颜色格式。
