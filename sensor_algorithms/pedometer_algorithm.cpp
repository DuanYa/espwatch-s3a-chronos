#include "pedometer_algorithm.h"
#include <math.h>

const float Pedometer::STEP_THRESHOLD = 0.15f; // G (suitable for typical walking/running)

Pedometer::Pedometer() {
    reset();
}

void Pedometer::reset() {
    stepCount = 0;
    bufferIndex = 0;
    lastFilterValue = 1.0f;
    peakValue = 1.0f;
    valleyValue = 1.0f;
    threshold = 1.1f;
    lastStepTime = 0;
    for (int i = 0; i < 5; i++) {
        sampleBuffer[i] = 1.0f;
    }
}

bool Pedometer::processSample(float ax, float ay, float az) {
    // 1. Calculate vector magnitude
    float magnitude = sqrt(ax * ax + ay * ay + az * az);

    // 2. Simple moving average filter to smooth the noise
    sampleBuffer[bufferIndex] = magnitude;
    bufferIndex = (bufferIndex + 1) % 5;

    float sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += sampleBuffer[i];
    }
    float filteredValue = sum / 5.0f;

    bool stepDetected = false;
    unsigned long currentTime = millis();

    // 3. Peak-valley dynamic tracking
    if (filteredValue > lastFilterValue) {
        // Signal is rising
        if (filteredValue > peakValue) {
            peakValue = filteredValue;
        }
    } else {
        // Signal is falling
        if (filteredValue < valleyValue) {
            valleyValue = filteredValue;
        }

        // Potential peak detected when signal starts to drop
        if (lastFilterValue == peakValue) {
            float amplitude = peakValue - valleyValue;

            // Check if peak is above dynamic threshold and amplitude is sufficient
            if (peakValue > threshold && amplitude > STEP_THRESHOLD) {
                // Time window validation to prevent false triggers (e.g. hand shaking)
                unsigned long interval = currentTime - lastStepTime;
                if (interval >= MIN_STEP_INTERVAL && interval <= MAX_STEP_INTERVAL) {
                    stepCount++;
                    stepDetected = true;
                    lastStepTime = currentTime;

                    // Dynamically adjust threshold
                    threshold = (peakValue + valleyValue) / 2.0f;
                } else if (interval > MAX_STEP_INTERVAL) {
                    // Reset if the interval is too long
                    lastStepTime = currentTime;
                }
            }
            valleyValue = peakValue; // Reset valley tracker
        }
    }

    lastFilterValue = filteredValue;
    return stepDetected;
}

uint32_t Pedometer::getStepCount() const {
    return stepCount;
}

void Pedometer::setStepCount(uint32_t count) {
    stepCount = count;
}
