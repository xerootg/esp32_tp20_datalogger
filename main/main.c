#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "can.h"
#include "diag.c"
#include "file_operations.h"

volatile uint16_t timeSec = 0;
volatile uint8_t time10MSec = 0;
uint8_t config[3][4];

void app_main()
{
    //setup RTC
    //setup SD card - for now it'll be stdout
    FILE * outfile = setup_fs();
    //setup CAN interface
    CAN_Open();
    while (1) {
        vwtp(&config[0], outfile, false);
    }
}
