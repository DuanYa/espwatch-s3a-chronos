#ifndef PEDOMETER_ALGORITHM_H_
#define PEDOMETER_ALGORITHM_H_

#include <Arduino.h>

class Pedometer {
public:
    Pedometer();
    void reset();

    // Process a single 3-axis accelerometer sample (in G or m/s^2)
    // Returns true if a step is detected
    bool processSample(float ax, float ay, float az);

    uint32_t getStepCount() const;
    void setStepCount(uint32_t count);

private:
    uint32_t stepCount;

    // Filtering and peak detection variables
    float sampleBuffer[5];
    uint8_t bufferIndex;

    float lastFilterValue;
    float peakValue;
    float valleyValue;
    float threshold;

    unsigned long lastStepTime;

    // Step detection parameters
    static const unsigned long MIN_STEP_INTERVAL = 250; // ms (minimum time between steps)
    static const unsigned long MAX_STEP_INTERVAL = 2000; // ms (maximum time between steps)
    static const float STEP_THRESHOLD; // Minimum peak-to-valley amplitude to trigger a step
};

#endif // PEDOMETER_ALGORITHM_H_
