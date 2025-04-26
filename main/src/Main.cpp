#include "freertos/FreeRTOS.h"

#include "EspDucky.hpp"
#include "Logger.hpp"


extern "C" void app_main(void)
{
    EspDucky espDucky{};
    ErrorCode err = espDucky.init();
    if(err != ErrorCode::Success) {
        LOGC("Failed to initialize EspDucky with error: %d. Aborting...", err);
    }

    // Enter infinite loop to keep the main task alive
    espDucky.run();

    fflush(stdout);
    esp_restart();
}