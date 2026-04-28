#include <Arduino.h>
#include "DynamentSensor.h"
#include "app.h"

DynamentSensor dynament(Serial1);

typedef enum
{
    INIT_START_SENSOR = 0,
    INIT_SEND_REQUEST,
    INIT_RECEIVE_DATA,
    INIT_FINISH
} DynamentTaskState;

static DynamentTaskState dynamentState = INIT_START_SENSOR;
static uint32_t receiveStartMs = 0;
static uint32_t finishStartMs = 0;
static const uint32_t responseTimeoutMs = 2000;
static const uint32_t cycleDelayMs = 2000;
static float gasValue = 0.0f;

void InitialiseDynamentSensorTask(void)
{
    switch (dynamentState)
    {
        case INIT_START_SENSOR:
            Serial.println("INIT_START_SENSOR");
            dynament.begin(9600);
            dynamentState = INIT_SEND_REQUEST;
            break;

        case INIT_SEND_REQUEST:
            Serial.println("INIT_SEND_REQUEST");

            if (dynament.sendLiveData2Request())
            {
                receiveStartMs = millis();
                dynamentState = INIT_RECEIVE_DATA;
            }
            else
            {
                Serial.println("Send failed");
                finishStartMs = millis();
                dynamentState = INIT_FINISH;
            }
            break;

        case INIT_RECEIVE_DATA:
            if (dynament.poll())
            {
                if (dynament.getResponse() == DynamentSensor::NEW_DATA ||
                    dynament.getResponse() == DynamentSensor::NEW_DATA_OUTLIER)
                {
                    gasValue = dynament.getLatestGasValue();

                    Serial.print("Gas Value 1: ");
                    Serial.println(gasValue, 3);

                    Serial.print("Gas Value 2: ");
                    Serial.println(dynament.getLatestGasValue2(), 3);

                    Serial.print("Status1: ");
                    Serial.println(dynament.getLatestStatus1());

                    Serial.print("Status2: ");
                    Serial.println(dynament.getLatestStatus2());
                }
                else
                {
                    Serial.print("Invalid response code: ");
                    Serial.println((int)dynament.getResponse());
                }

                finishStartMs = millis();
                dynamentState = INIT_FINISH;
            }
            else if ((millis() - receiveStartMs) > responseTimeoutMs)
            {
                Serial.println("Timeout waiting for sensor response");
                finishStartMs = millis();
                dynamentState = INIT_FINISH;
            }
            break;

        case INIT_FINISH:
            if ((millis() - finishStartMs) >= cycleDelayMs)
            {
                Serial.println("----------------------");
                dynamentState = INIT_SEND_REQUEST;
            }
            break;

        default:
            dynamentState = INIT_START_SENSOR;
            break;
    }
}
