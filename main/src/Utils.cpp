#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Utils.hpp"

namespace Utils
{
    void delay(uint32_t ms) {
        vTaskDelay(ms / portTICK_PERIOD_MS);
    }
}