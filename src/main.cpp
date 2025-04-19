#include <Arduino.h>
#include "Crsf_Routine.h"
#include "HMI_Routine.h"

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("- SimpleRCU\r\n");
    Serial.printf("- @build at %s %s\r\n", __TIME__, __DATE__);

    Boot_screen();
    Crsf_setup();
    Disp_setup();
}

void loop()
{
}
