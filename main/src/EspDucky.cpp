#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include <cJSON.h>

#include "EspDucky.hpp"
#include "Logger.hpp"
#include "Script.hpp"
#include "StaticWebData.hpp"

#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default

EspDucky::EspDucky() : 
ap("esp-ducky", "ducky123"), 
mdns("esp-ducky"), 
http(std::unordered_map<std::string, HttpServer::StaticEndpoint>{
    {"/", {
            .respBuf = STATIC_WEB_DATA_PTR(index_html), 
            .respLen = STATIC_WEB_DATA_SIZE(index_html),
            .mime = "text/html"
        }
    },
    {"/script.js", {
            .respBuf = STATIC_WEB_DATA_PTR(script_js), 
            .respLen = STATIC_WEB_DATA_SIZE(script_js),
            .mime = "text/javascript"
        }
    },
    {"/style.css", {
        .respBuf = STATIC_WEB_DATA_PTR(style_css), 
        .respLen = STATIC_WEB_DATA_SIZE(style_css),
        .mime = "text/css"
        }
    },
    {"/favicon.ico", {
        .respBuf = STATIC_WEB_DATA_PTR(favicon_ico), 
        .respLen = STATIC_WEB_DATA_SIZE(favicon_ico),
        .mime = "image/x-icon"
        }
    },
},std::unordered_map<std::string, std::function<ErrorCode(const std::string&, std::string&)>>{
    {"/script", [this](const std::string &request, std::string &response) {
            return handleScriptEndpoint(request, response);
        }
    },
    {"/config", nullptr}
}), 
usb() {}

ErrorCode EspDucky::init() {
    LOGD("EspDucky initialization...");

    // Initialize BOOT button 
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = (gpio_pullup_t)true,
        .pull_down_en = (gpio_pulldown_t)false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&boot_button_config);
    if (ret) {
        LOGC("Failed to configure BOOT button with error: %d. Aborting...", ret);
    }

    //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        LOGW("Erasing NVS flash due to %s", ret == ESP_ERR_NVS_NO_FREE_PAGES ? "lack of free pages" : "new NVS version");
        ret = nvs_flash_erase();
        if(ret) {
            LOGC("Failed to erase NVS flash with error: %d. Aborting...", ret);
        }
        ret = nvs_flash_init();
    }
    if(ret) {
        LOGC("Failed to initialize NVS flash with error: %d. Aborting...", ret);
    }


    ap.start();
    mdns.start();
    http.start();
    usb.start();

    return ErrorCode::Success;
}

void EspDucky::run() {
    uint8_t usbDeviceDisableCountdown = 3;

    // Infinite loop to keep the task running
    for (;;) {
        if(usb.isStarted()) {
            if(!gpio_get_level(APP_BUTTON)){
                if(usbDeviceDisableCountdown == 0) {
                    LOGI("Disabling USB Device");
                    usb.stop();
                    LOGI("Enabling serial JTAG");
                    usb.enableJTAG();
                }
                else{
                    LOGI("USB will be disabled in %d seconds", usbDeviceDisableCountdown);
                    usbDeviceDisableCountdown--;
                }
            } 
            else {
                usbDeviceDisableCountdown = 3;
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

ErrorCode EspDucky::handleScriptEndpoint(const std::string &request, std::string &response) {
    cJSON *reqJson = cJSON_Parse(request.data());
    if (!reqJson) {
        LOGE("Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return ErrorCode::GeneralError;
    }

    cJSON *scriptJson = cJSON_GetObjectItemCaseSensitive(reqJson, "script");
    if (!cJSON_IsString(scriptJson) || (scriptJson->valuestring == NULL)) {
        LOGE("Invalid JSON format: 'script' is not a string");
        cJSON_Delete(reqJson);
        return ErrorCode::GeneralError;
    }

    cJSON *actionJson = cJSON_GetObjectItemCaseSensitive(reqJson, "action");
    if (!cJSON_IsNumber(actionJson)) {
        LOGE("Invalid JSON format: 'action' is not a number");
        cJSON_Delete(reqJson);
        return ErrorCode::GeneralError;
    }

    LOGD("Parsed script: '%s'", scriptJson->valuestring);
    LOGD("Parsed action: '%d'", actionJson->valueint);

    auto script = Script::parse(scriptJson->valuestring);
    cJSON_Delete(reqJson); // Free the request json object

    LOGD("Script parsing successful");

    if (!script) {
        LOGE("Failed to parse script: %s", scriptJson->valuestring);
        return ErrorCode::GeneralError;
    }

    if(usb.isMounted()) {
        if (script->run(usb) != ErrorCode::Success) {
            LOGE("Failed to run script: %s", scriptJson->valuestring);
            return ErrorCode::GeneralError;
        }
    }
    else
    {
        LOGW("USB device not mounted. Skipping script execution.");
    }

    // Prepare response
    cJSON *respJson = cJSON_CreateObject();
    if (!respJson) {
        LOGE("Failed to create JSON response object");
        return ErrorCode::GeneralError;
    }

    cJSON_AddStringToObject(respJson, "status", "success");

    char *respJsonStr = cJSON_PrintUnformatted(respJson);
    response = respJsonStr;

    std::free(respJsonStr); // Free the JSON string
    cJSON_Delete(respJson); // Free the response json object

    return ErrorCode::Success;    
}
