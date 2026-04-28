

// MKR1010:
// Serial  = USB serial monitor
// Serial1 = hardware UART
// User requested: pin 14 = TX, pin 13 = RX

#include <Arduino.h>
#include "app.h"

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    Serial.println("Main started");
}

void loop()
{
    InitialiseDynamentSensorTask();
}
