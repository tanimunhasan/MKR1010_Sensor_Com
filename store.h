/*
 * store.h
 *
 *  Created on: 28 Apr 2026
 *      Author: B4T
 */

#ifndef STORE_H_
#define STORE_H_


#include <Arduino.h>

typedef struct
{
    float gasValue1;
    float gasValue2;
    uint16_t status1;
    uint16_t status2;
    uint32_t timestampMs;
    bool valid;
} SensorReading_t;

void Store_Init(void);
void Store_Clear(void);

void Store_UpdateDynamentReading(float gas1,
                                 float gas2,
                                 uint16_t status1,
                                 uint16_t status2);

bool Store_GetLatestReading(SensorReading_t *reading);
bool Store_HasValidReading(void);


#endif /* STORE_H_ */
