#include <stdio.h>
#include "freertos/FreeRTOS.h"

void app_main() {
    while (true) {
        printf("Hello World!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}