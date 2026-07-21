/*
 Optical Heart Rate Detection (PBA Algorithm)
 By: Nathan Seidle
 SparkFun Electronics
 Date: October 2nd, 2016

 Given a series of IR samples from the MAX30105 we discern when a heart beat is occurring
*/

#ifndef HEART_RATE_H_
#define HEART_RATE_H_

#include <Arduino.h>

bool checkForBeat(int32_t sample);
int16_t averageDCEstimator(int32_t *p, uint16_t x);
int16_t lowPassFIRFilter(int16_t din);
int32_t mul16(int16_t x, int16_t y);

#endif // HEART_RATE_H_
