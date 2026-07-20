# Waveshare ESP32-S3-Touch-LCD-1.69 (ESPS3_1_69) 引脚接线说明

本文档分析了当前项目中默认编译环境（`lolin_s3_mini_1_69` / `ESPS3_1_69`）下的ESP32-S3芯片与各项外设的引脚连接关系。

## 1. 屏幕 (SPI 接口)
使用的是 SPI3_HOST。

| 外设/功能 | ESP32-S3 引脚 | 说明 |
| :--- | :--- | :--- |
| SCLK / SCK | GPIO 7 | SPI 时钟 |
| MOSI | GPIO 5 | SPI 数据输入 (主出从入) |
| MISO | -1 | 未使用 |
| DC | GPIO 6 | 数据/命令控制 |
| CS | GPIO 8 | 片选 |
| RST | GPIO 4 | 复位引脚 |
| BL | GPIO 14 | 背光控制 |

## 2. 触摸屏 (I2C 接口)
使用的控制器为 FT6X36，I2C 地址 0x38 (根据代码初始化设置)。

| 外设/功能 | ESP32-S3 引脚 | 说明 |
| :--- | :--- | :--- |
| I2C_SDA | GPIO 1 | I2C 数据 |
| I2C_SCL | GPIO 2 | I2C 时钟 |
| TP_INT | GPIO 10 | 触摸中断引脚 |
| TP_RST | GPIO 11 | 触摸复位引脚 |

## 3. 其他外设

| 外设/功能 | ESP32-S3 引脚 | 说明 |
| :--- | :--- | :--- |
| VIBRATION_PIN | GPIO 9 | 震动马达 |
| BUTTON_HOME | GPIO 0 | 主页按键 (Boot 按键) |

> 备注：以上信息提取自 `hal/esp32/displays/pins.h` 及 `hal/esp32/app_hal.cpp`。
