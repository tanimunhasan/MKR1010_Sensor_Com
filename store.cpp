/*
 * store.cpp
 *
 *  Created on: 28 Apr 2026
 *      Author: B4T
 */
#include<Arduino.h>
#include "DynamentSensor.h"
#include "store.h"
#include "app.h"
static SensorReading_t latestReading;

void Store_Init(void)
{
    Store_Clear();
}

void Store_Clear(void)
{
    latestReading.gasValue1 = 0.0f;
    latestReading.gasValue2 = 0.0f;
    latestReading.status1 = 0;
    latestReading.status2 = 0;
    latestReading.timestampMs = 0;
    latestReading.valid = false;
}

void Store_UpdateDynamentReading(float gas1,
                                 float gas2,
                                 uint16_t status1,
                                 uint16_t status2)
{
    latestReading.gasValue1 = gas1;
    latestReading.gasValue2 = gas2;
    latestReading.status1 = status1;
    latestReading.status2 = status2;
    latestReading.timestampMs = millis();
    latestReading.valid = true;
}

bool Store_GetLatestReading(SensorReading_t *reading)
{
    if (reading == nullptr)
    {
        return false;
    }

    if (!latestReading.valid)
    {
        return false;
    }

    *reading = latestReading;
    return true;
}

bool Store_HasValidReading(void)
{
    return latestReading.valid;
}


