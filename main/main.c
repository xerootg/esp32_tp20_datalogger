#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "can.h"
#include "vwtp.h"

void app_main(void)
{
    //setup RTC
    //setup SD card
    //setup CAN interface
    CAN_Open();
    while (1) {

    }
}
