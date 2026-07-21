# 智能穿戴传感器核心算法代码库 (Sensor Algorithms Library)

本文件夹整理并集成了智能手表/手环等穿戴设备中最为核心的三个健康与运动监测算法的原始实现代码：
1. **心率检测算法** (Heart Rate Detection)
2. **血氧饱和度计算算法** (SpO2 Calculation)
3. **三轴加速度计软件记步算法** (3-Axis Accelerometer Pedometer)

---

## 一、 文件结构与代码详情说明

### 1. 心率检测算法 (`heart_rate.h`, `heart_rate.cpp`)
- **核心算法**：Maxim 经典的 **PBA (Peripheral Beat Amplitude，外周脉搏振幅)** 算法。
- **算法细节**：
  - **DC 平均估算器 (`averageDCEstimator`)**：采用一阶 IIR 低通滤波器实时估算 PPG 信号中的直流 (DC) 分量（即血管随呼吸或基线漂移产生的缓慢漂移量），并从原始信号中滤除，获得纯净的交流 (AC) 信号。
  - **低通有限冲激响应滤波器 (`lowPassFIRFilter`)**：对 AC 信号使用 12 阶线性相位 FIR 滤波器进行平滑去噪，滤除高频肌肉颤动与电磁噪声。
  - **零交叉波峰检测 (`checkForBeat`)**：在信号正向过零点（上升沿）和反向过零点（下降沿）时动态寻找信号的局部最大值（Peak）与最小值（Valley）。当峰峰值（振幅）在合理窗口范围内时，判定为一次有效的心跳（Beat），并触发检测成功标志。
  - **安全性警示**：本算法仅供开发学习、学习滤波设计以及非医疗级的玩具或原型项目。

### 2. 血氧饱和度算法 (`spo2_algorithm.h`, `spo2_algorithm.cpp`)
- **核心算法**：基于红光 (Red) 与红外光 (IR) 光电容积脉搏波的**双波长消光比 (Ratio of Ratios)** 测量法。
- **算法细节**：
  - **DC 漂移滤除**：计算 100 个样本（采集自红光与红外光通道，采样频率为 25Hz）的均值，转换为无直流偏移的相对波动信号，并使用 4 点滑动平均滤波器进行降噪。
  - **波谷定位与频率计算 (`maxim_find_peaks`)**：使用反转信号的峰值检测器，计算两次连续波谷之间的时间间隔（PPG 周期），从而获得平均心率。
  - **AC/DC 比例计算**：寻找每个心搏周期内的红外通道与红光通道的 AC 交流波动分量与静态 DC 分量之比：
    $$\text{Ratio} = \frac{(AC_{\text{Red}} / DC_{\text{Red}})}{(AC_{\text{IR}} / DC_{\text{IR}})}$$
  - **查表换算 SpO2 (`uch_spo2_table`)**：由于浮点运算与二次曲线拟合在低功耗嵌入式处理器中开销较大，本算法采用预先标定好的拟合二次曲线查表。公式近似为：
    $$\text{SpO2} = -45.06 \times \text{Ratio}^2 + 30.354 \times \text{Ratio} + 94.845$$

### 3. 记步算法 (`pedometer_algorithm.h`, `pedometer_algorithm.cpp`)
- **核心算法**：基于三轴加速度计的**动态阈值与时间窗口波峰检测软件记步算法**。
- **算法细节**：
  - **向量合速度计算**：将三轴（X、Y、Z）加速度合并为一个不依赖摆放方向的合加速度幅值向量 $S$（消除由于手腕旋转带来的重力分量偏差）：
    $$S = \sqrt{a_x^2 + a_y^2 + a_z^2}$$
  - **滑动均值平滑 (Moving Average)**：内置 5 样本深度的环形队列，对 $S$ 运行均值滤波，消除手臂细微颤抖或传感器白噪声产生的毛刺。
  - **动态阈值更新 (Dynamic Threshold)**：算法自动寻找合速度在一段时间内的峰值 (Peak) 与谷值 (Valley)。阈值设定在峰谷两端的中点处（即 $\text{Threshold} = \frac{\text{Peak} + \text{Valley}}{2}$）。只有当最新采集的峰值超过该动态阈值，且峰谷差值（波幅）大于最低触发限（`STEP_THRESHOLD`，通常设为 0.15G）时，方可被判定为一次潜在的踏步。
  - **时间窗口校验 (Time Window Filter)**：为过滤非步行振动（如敲击键盘、乘车颠簸或挥手），引入时间间隔约束（250ms $\le \Delta t \le$ 2000ms，对应每秒 0.5 到 4 步）。若检测信号落在该合法区间内，步数加 1，否则视为无效高频抖动或异常振动，予以丢弃。

---

## 二、 代码使用与集成说明

### 1. 心率与血氧算法集成示例
以下代码展示了如何配合光学传感器库（如 `MAX30105` 驱动）读取数据并驱动上述算法。

```cpp
#include <Wire.h>
#include "heart_rate.h"
#include "spo2_algorithm.h"

// 传感器读取数据缓存
uint32_t redBuffer[BUFFER_SIZE]; // 100 长度
uint32_t irBuffer[BUFFER_SIZE];  // 100 长度

int32_t spo2;           // 输出血氧百分比
int8_t isValidSpO2;     // 血氧计算是否有效
int32_t heartRate;      // 算法心率输出
int8_t isValidHR;       // 心率计算是否有效

void setup() {
    Serial.begin(115200);
    Wire.begin(1, 2); // 传入 SDA=1, SCL=2 (根据当前 ESPS3_1_69 设计)

    // 初始化并启动传感器
    // particleSensor.begin(Wire, 400000);
    // particleSensor.setup();
}

void loop() {
    // 1. 单次心跳检测模式 (实时性强，适合 UI 动画和瞬时心率)
    // uint32_t irValue = particleSensor.getIR();
    // if (checkForBeat(irValue) == true) {
    //     Serial.println("检测到一次瞬时心搏！");
    // }

    // 2. 批量血氧饱和度计算模式 (周期性检测，如每10秒计算一次)
    static unsigned long lastMeasureTime = 0;
    if (millis() - lastMeasureTime > 10000) {
        // 采集 100 个样本数据
        for (int i = 0; i < BUFFER_SIZE; i++) {
            // redBuffer[i] = particleSensor.getFIFORed();
            // irBuffer[i] = particleSensor.getFIFOIR();
            // particleSensor.nextSample();
            delay(40); // 对应 25Hz 的采样速率
        }

        // 执行血氧和心率算法
        maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer,
                                               &spo2, &isValidSpO2,
                                               &heartRate, &isValidHR);

        if (isValidSpO2 && isValidHR) {
            Serial.printf("平均心率: %d BPM, 血氧浓度 (SpO2): %d%%\n", heartRate, spo2);
        } else {
            Serial.println("接触不佳或信号干扰，请重试！");
        }
        lastMeasureTime = millis();
    }
}
```

### 2. 软件记步算法集成示例
以下代码展示了如何获取三轴加速度计的实时数据，并将其送入 `Pedometer` 算法计算步数。

```cpp
#include "pedometer_algorithm.h"

Pedometer pedometer;

// 模拟或从传感器读取加速度计数据
void readAndProcessAccelerometer() {
    float ax, ay, az;

    // 假设从 IMU（如 QMI8658 或 LSM6DS3）读取单位为 G 的加速度值
    // imu.getAccel(&ax, &ay, &az);

    // 输入样本数据进行算法处理
    bool isStep = pedometer.processSample(ax, ay, az);

    if (isStep) {
        uint32_t currentSteps = pedometer.getStepCount();
        Serial.printf("🏃 检测到迈步！当前总步数: %u 步\n", currentSteps);
    }
}

void setup() {
    Serial.begin(115200);
    pedometer.reset(); // 初始化/重置记步数据
}

void loop() {
    // 推荐保持固定采样间隔，如 50Hz (即每 20ms 采样处理一次)
    static unsigned long lastSampleTime = 0;
    if (millis() - lastSampleTime >= 20) {
        readAndProcessAccelerometer();
        lastSampleTime = millis();
    }
}
```

---

## 三、 算法移植与性能调优建议

1. **传感器采样率的一致性**：
   - 算法中的 FIR 滤波、时间窗口等均基于特定的采样频率。心率/血氧算法强依赖于 **25Hz** 采样频率；记步算法最佳输入频率范围为 **50Hz - 100Hz**。移植到其他主控或传感器时，请务必保证硬件采样定时与算法设定相符。
2. **重力加速度单位**：
   - 记步算法中的输入 `ax`, `ay`, `az` 单位应为 **G**（即重力加速度单位，1G $\approx$ 9.8 $m/s^2$）。若传感器读出的原始数值是 16位 raw 原始值，需根据传感器配置的量程范围（如 $\pm2\text{G}$ 或 $\pm4\text{G}$）转换后再送入算法。
3. **低功耗休眠策略**：
   - 在进入深度睡眠（Deep Sleep）或轻度睡眠（Light Sleep）时，可以让三轴传感器工作在特定的“低功耗中断触发”模式（例如开启 IMU 芯片内部的 Single Tap 或 Motion Detect 中断），唤醒主 CPU 后再启动高精度的软件记步，以最大程度地节省穿戴设备的电池电量。
