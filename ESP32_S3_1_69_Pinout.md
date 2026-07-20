# ESP32-S3-Touch-LCD-1.69 (ESPS3_1_69) 外设引脚接线与使用详细说明

本文档详细分析并整理了当前项目中默认编译环境（`lolin_s3_mini_1_69` / `ESPS3_1_69`）下的 ESP32-S3 芯片与各项板载、外接外设的引脚连接关系、硬件参数（如屏幕分辨率、通信总线参数等）以及关键外设的使用方法。

所有引脚映射与参数均与代码中的定义（如 `hal/esp32/displays/pins.h` 和 `hal/esp32/app_hal.cpp`）保持完全一致。

---

## 1. 屏幕 (通信与参数)

### 1.1 屏幕硬件参数
- **驱动芯片**：ST7789V2
- **屏幕分辨率**：**240 × 280** 像素 (矩形屏)
- **屏幕偏移配置** (对应 `pins.h` 宏)：
  - `OFFSET_X` = 0
  - `OFFSET_Y` = 20
- **像素颜色格式**：RGB565，RGB顺序为 `false` (BGR模式)

### 1.2 通信使用方式
屏幕驱动通过 **SPI3_HOST (SPI3)** 串行总线进行数据的高速通信。为了保证界面的极致流畅，固件配合 **LovyanGFX** 硬件驱动库和 **LVGL 9** 的双缓存机制，启用了 **DMA (Direct Memory Access)** 模式，时钟频率设定为 **40MHz (写)** 及 **20MHz (读)**。

#### SPI 物理引脚接线：

| 外设/功能 | ESP32-S3 引脚 | 代码宏定义 | 说明 |
| :--- | :--- | :--- | :--- |
| **SCLK / SCK** | GPIO 7 | `SCLK` | SPI 时钟信号 |
| **MOSI / DIN** | GPIO 5 | `MOSI` | SPI 数据输入 (主出从入) |
| **MISO / DOUT**| -1 (未使用) | `MISO` | 未使用 |
| **DC** | GPIO 6 | `DC` | 数据 / 命令选择控制引脚 |
| **CS** | GPIO 8 | `CS` | 片选控制引脚 |
| **RST** | GPIO 4 | `RST` | 复位引脚 |
| **BL (背光)** | GPIO 14 | `BL` | LCD 背光控制（使用 PWM 驱动支持多级亮度调节） |

---

## 2. 触摸屏 (I2C 接口)
触摸屏使用 **FT6X36** 电容式触摸控制器，I2C 7位物理地址为 `0x38`。

| 外设/功能 | ESP32-S3 引脚 | 代码宏定义 | 说明 |
| :--- | :--- | :--- | :--- |
| **I2C_SDA** | GPIO 1 | `I2C_SDA` | I2C 数据线 |
| **I2C_SCL** | GPIO 2 | `I2C_SCL` | I2C 时钟线 |
| **TP_INT** | GPIO 10 | `TP_INT` | 触摸中断引脚 |
| **TP_RST** | GPIO 11 | `TP_RST` | 触摸复位引脚 |

---

## 3. 惯性测量单元 IMU (I2C 接口)
板载集成了 6 轴运动传感器 **QMI8658C**（包含 3 轴加速度计 + 3 轴陀螺仪），I2C 地址为 `0x6B`。
它与触摸屏等 I2C 设备共享同一个 I2C 硬件总线。

| 外设/功能 | ESP32-S3 引脚 | 代码对应 | 说明 |
| :--- | :--- | :--- | :--- |
| **I2C_SDA** | GPIO 1 | `I2C_SDA` | 共享 I2C 数据线 |
| **I2C_SCL** | GPIO 2 | `I2C_SCL` | 共享 I2C 时钟线 |

---

## 4. 心率血氧传感器 (I2C 接口)

### 4.1 硬件连接
系统支持通过共享的 I2C 硬件总线连接 **MAX30105** / **MAX30102** 光学心率与血氧饱和度传感器，其 I2C 地址为 `0x57`。

| 外设/功能 | ESP32-S3 引脚 | 代码对应 | 说明 |
| :--- | :--- | :--- | :--- |
| **I2C_SDA** | GPIO 1 | `I2C_SDA` | 共享 I2C 数据线 |
| **I2C_SCL** | GPIO 2 | `I2C_SCL` | 共享 I2C 时钟线 |

### 4.2 传感器使用方法 (C++ 代码示例)
项目中集成了专用的心率和血氧计算驱动库（位于 `hal/esp32/drivers/` 目录下）。

#### 1) 初始化传感器
```cpp
#include <Wire.h>
#include "drivers/MAX30105.h"

MAX30105 particleSensor;

void initHeartRateSensor() {
    // 1. 初始化共享的 I2C 接口
    Wire.begin(1, 2);
    Wire.setClock(400000); // 启用 400kHz I2C 快速模式

    // 2. 检查传感器并初始化
    if (!particleSensor.begin(Wire, 400000)) {
        Serial.println("未找到 MAX30105/102 传感器，请检查接线和供电！");
        return;
    }

    // 3. 配置传感器参数用于检测心率与血氧
    // 默认配置：LED功率=0x1F, 样本平均=4, LED模式=3(多LED全开), 采样率=400, 脉宽=411, ADC量程=4096
    particleSensor.setup();
}
```

#### 2) 心率检测 (Beat Detection)
利用 `drivers/heartRate.h` 提供的 Maxim 经典 PBA 算法检测实时心率：
```cpp
#include "drivers/heartRate.h"

long lastBeat = 0;
float beatsPerMinute;

void updateHeartRate() {
    // 读取红外(IR)通道值 (红外光对血液脉动最敏感)
    uint32_t irValue = particleSensor.getIR();

    // 运行滤波并检测脉搏波峰
    if (checkForBeat(irValue) == true) {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            Serial.printf("检测到心跳！实时 BPM: %.1f\n", beatsPerMinute);
        }
    }
}
```

#### 3) 血氧饱和度 (SpO2) 测量
利用 `drivers/spo2_algorithm.h` 提供的双波长比值计算血氧（利用红光与红外光的消光比差异）：
```cpp
#include "drivers/spo2_algorithm.h"

uint32_t irBuffer[BUFFER_SIZE];   // IR 传感器数据缓存 (25Hz采样频率)
uint32_t redBuffer[BUFFER_SIZE];  // 红光传感器数据缓存

int32_t spo2;                     // 计算出的血氧饱和度百分比 (0-100)
int8_t isValidSpO2;               // 血氧是否有效标志 (1为有效，0为无效)
int32_t heartRate;                // 算法计算出的平均心率
int8_t isValidHR;                 // 心率是否有效标志

void measureSpO2() {
    // 1. 采集 BUFFER_SIZE (通常为 100) 个数据样本
    for (byte i = 0 ; i < BUFFER_SIZE ; i++) {
        while (particleSensor.available() == false) {
            particleSensor.check(); // 查询传感器是否有新数据
        }

        redBuffer[i] = particleSensor.getFIFORed(); // 读取红光值
        irBuffer[i] = particleSensor.getFIFOIR();   // 读取红外光值
        particleSensor.nextSample();                 // 移至下一样本
    }

    // 2. 调用算法进行计算
    maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer,
                                           &spo2, &isValidSpO2,
                                           &heartRate, &isValidHR);

    if (isValidSpO2) {
        Serial.printf("计算成功！血氧饱和度 (SpO2): %d%%\n", spo2);
    } else {
        Serial.println("血氧数据无效，请保持手指静止。");
    }
}
```

---

## 5. 音频解码芯片与喇叭 (I2C + I2S 接口)
系统采用 **ES8311** 低功耗单声道音频 Codec 芯片用于实现音频的解码与播放控制。

### 5.1 控制接口 (I2C)
ES8311 芯片的控制引脚（用于设置音量、配置采样率等）通过 I2C 接口连接，I2C 7位地址为 `0x18`。

| 外设/功能 | ESP32-S3 引脚 | 说明 |
| :--- | :--- | :--- |
| **I2C_SDA** | GPIO 1 | 共享 I2C 数据线 |
| **I2C_SCL** | GPIO 2 | 共享 I2C 时钟线 |

### 5.2 音频数据接口 (I2S)
音频数字信号通过 I2S 数据总线进行双向传输，以下为 I2S 接口的典型推荐连接：

| 功能信号 | 推荐引脚 | 说明 |
| :--- | :--- | :--- |
| **MCLK (主时钟)** | GPIO 3 / GPIO 16 | 音频芯片主时钟（供同步使用） |
| **BCLK (位时钟)** | GPIO 15 / GPIO 9 | I2S 通信的位同步时钟信号 |
| **LRCK / WS** | GPIO 16 / GPIO 10 | I2S 帧时钟 / 声道选择信号 |
| **I2S_DOUT / DACDAT**| GPIO 17 / GPIO 11 | ESP32-S3 输出音频数据至 ES8311 进行喇叭播放 |
| **I2S_DIN / ADCDAT** | GPIO 18 / GPIO 12 | ES8311 输入录音数据至 ESP32-S3 |

### 5.3 喇叭 (Speaker) 连接
喇叭通常不直接连接至 ESP32-S3 的 GPIO，而是通过 ES8311 的模拟输出引脚（如 `OUT1P`/`OUT1N` 或 `LOUT`/`ROUT`）连接到音频功放芯片（例如 NS4150 或 PAM8302），功放芯片再驱动喇叭进行声音播放。

---

## 6. 麦克风 (MIC)
麦克风采集支持以下两种方案：

### 6.1 方案 A：模拟麦克风（通过 ES8311 解码）
模拟硅麦或驻极体麦克风连接到 **ES8311** 芯片的模拟输入端（`MIC1P`/`MIC1N`），信号经芯片内部的低噪声放大器（PGA）和高精度 ADC 数字化后，通过 I2S 总线（**I2S_DIN**）输入给 ESP32-S3 芯片。

### 6.2 方案 B：数字 I2S 麦克风（如 INMP441 / MSM261S4030H0）
可以直接连接到 ESP32-S3 的 I2S 硬件接口引脚：

| 麦克风信号 | ESP32-S3 连接引脚 | 说明 |
| :--- | :--- | :--- |
| **SCK (BCLK)** | GPIO 15 / GPIO 9 | 时钟信号 |
| **WS (LRCK)** | GPIO 16 / GPIO 10 | 声道选择 / 帧同步 |
| **SD (SDIN)** | GPIO 18 / GPIO 12 | 串行音频数据输出 |

---

## 7. 按键
开发板包含用于用户交互的实体按键：

| 按键功能 | ESP32-S3 引脚 | 代码宏定义 | 软件处理与逻辑 |
| :--- | :--- | :--- | :--- |
| **BUTTON_HOME (主页/唤醒按键)**| GPIO 0 | `BUTTON_HOME` | - **单击**：唤醒屏幕 / 返回主页<br>- **双击**：打开应用列表<br>- **长按**：切换内置表盘面<br>- **休眠唤醒**：配置为 ext0 唤醒源 |

---

## 8. 震动马达
用于提供触觉反馈和各类通知提醒。

| 外设/功能 | ESP32-S3 引脚 | 代码宏定义 | 说明 |
| :--- | :--- | :--- | :--- |
| **VIBRATION_PIN** | GPIO 9 | `VIBRATION_PIN` | 震动马达控制引脚。输出高电平（High）开启震动，输出低电平（Low）停止震动。 |

---

## 9. 其它系统外设

| 外设名称 | 设备型号 | 接口 / 引脚 | 说明 |
| :--- | :--- | :--- | :--- |
| **RTC 实时时钟** | PCF85063 | I2C (地址: `0x51`) <br> SCL: GPIO 2 / SDA: GPIO 1 | 用于在系统主 CPU 深度休眠时保持高精度时间同步。 |
| **电池电压采样 (ADC)**| 分压电路 | **BAT_ADC**: GPIO 1 | 配合分压电阻电路通过 ADC 通道采集电压。 |

---

> **开发说明与注意事项：**
> 1. 上述引脚配置完全符合 `hal/esp32/displays/pins.h` 中编译宏 `#elif ESPS3_1_69` 下的代码定义。
> 2. 所有 I2C 外设（包括触摸屏、IMU、心率传感器、音频解码芯片、RTC等）均挂载在同一组由 `I2C_SDA (GPIO 1)` 和 `I2C_SCL (GPIO 2)` 组成的共享 I2C 硬件总线上。
