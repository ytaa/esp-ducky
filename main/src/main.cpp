#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "WiFiAccessPoint.hpp"
#include "Logger.hpp"
#include "HttpServer.hpp"

extern "C" void app_main(void)
{
    LOGD("Initializing...");

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create an Access Point
    WiFiAccessPoint ap("esp-ducky", "ducky123");
    ap.start();

    HttpServer http{};
    http.start();

    // Infinite loop to keep the task running
    for (unsigned int i = 0;; i++) {
        LOGD("Running for %d seconds", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    LOGI("Restarting...");
    fflush(stdout);
    esp_restart();
}
