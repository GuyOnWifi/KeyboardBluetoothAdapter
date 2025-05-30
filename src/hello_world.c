<<<<<<< HEAD
#include <stdio.h>
#include "freertos/FreeRTOS.h"

void app_main() {
    while (true) {
=======
/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"

void app_main(void)
{
    bool state = false;
    while (true) {
        
>>>>>>> 056ded2 (Changed files)
        printf("Hello World!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}