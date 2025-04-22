#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"
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
nvConfig(ArmingState::Unarmed, UsbDeviceType::Hid), 
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

    // Open NVS handle
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("esp-ducky", NVS_READWRITE, &ret);
    if(ESP_OK != ret) {
        LOGC("Failed to open NVS handle with error: (%s). Aborting...", esp_err_to_name(ret));
    }

    // Read and handle the nvConfig and nvScript from NVS
    handleNvConfig(handle.get());
    handleNvScript(handle.get());

    ap.start();
    mdns.start();
    http.start();
    
    return ErrorCode::Success;
}

void EspDucky::handleNvConfig(nvs::NVSHandle *handle) {
    if(!handle) {
        LOGC("Failed to read the nvConfig - NVS handle is null. Aborting...");
    }

    // Read nvConfig from NVS
    esp_err_t ret = handle->get_blob("nvConfig", &nvConfig, sizeof(nvConfig));
    if(ESP_ERR_NVS_NOT_FOUND == ret)
    {
        LOGW("NVS data for nvConfig not found. Using default values.");
    }
    else if(ESP_OK != ret) {
        LOGC("Failed to retrieve NVS data with error: (%s). Aborting...", esp_err_to_name(ret));
    }

    // Handle USB device configuration
    switch(nvConfig.usbDeviceType) {
        case UsbDeviceType::SerialJtag: {
            LOGI("Starting USB Serial JTAG device...");
            // No action needed, as the USB Serial JTAG device is started by default
            break;
        }
        case UsbDeviceType::Hid: {
            LOGI("Starting USB HID device...");
            usb.start();
            break;
        }
        case UsbDeviceType::Msd: {
            LOGW("USB MSD device is not supported yet. Starting Serial JTAG device instead...");
            // No action needed, as the USB Serial JTAG device is started by default
            break;
        }
        case UsbDeviceType::HidMsd: {
            LOGW("USB MSD device is not supported yet. Starting only HID device instead...");
            usb.start();
            break;
        }
        default: {
            LOGC("Invalid USB device type in nvConfig data. Aborting...");
        }
    }
}

void EspDucky::handleNvScript(nvs::NVSHandle *handle) {
    if(nvConfig.armingState == ArmingState::Unarmed) {
        LOGI("Device is unarmed. No script is executed at startup.");
        return;
    }

    if(UsbDeviceType::Hid != nvConfig.usbDeviceType && UsbDeviceType::HidMsd != nvConfig.usbDeviceType) {
        LOGW("Device is not in HID mode. Script execution is not possible.");
        return;
    }

    LOGI("Device is armed with HID enabled. Reading script from NVS...");
    
    // Read script size from NVS
    uint32_t nvScriptSize = 0u;
    std::unique_ptr<uint8_t[]> nvScriptData{nullptr};

    esp_err_t ret = handle->get_item("nvScriptSize", nvScriptSize);
    if(ESP_OK == ret)
    {
        // Read script data from NVS
        nvScriptData = std::make_unique<uint8_t[]>(nvScriptSize);
        ret = handle->get_blob("nvScriptData", nvScriptData.get(), nvScriptSize);
    }

    // Check if script size and data was found in the NVS
    if(ESP_ERR_NVS_NOT_FOUND == ret) {
        LOGW("Device is armed but no script was stored in the NVS. The armed state is ignored.");
    }
    else if(ESP_OK != ret)
    {
        LOGC("Failed to retrieve script from NVS with error: (%s). Aborting...", esp_err_to_name(ret));
    }
    else {
        // Parse the script
        auto nvScript = Script::deserialize(std::span<const uint8_t>(nvScriptData.get(), nvScriptSize));
        if (!nvScript) {
            LOGE("Failed to deserialize script from NVS. The armed state is ignored.");
            return;
        }
        else {
            if(!usb.isMounted()) {
                LOGW("USB device not mounted during startup. The armed state is ignored.");
                return;
            }

            // Run the script
            if (ErrorCode::Success != nvScript->run(usb)) {
                LOGE("Failed to run script from NVS. The armed state is ignored.");
                return;
            }

            LOGI("Script from NVS executed successfully.");

            // Handle single run armed state
            if(ArmingState::Persistent == nvConfig.armingState) {
                LOGI("Device is armed in persistent mode. The armed state is not reset.");
                return;
            }

            // Reset the armed state to unarmed
            nvConfig.armingState = ArmingState::Unarmed;
            ret = handle->set_blob("nvConfig", &nvConfig, sizeof(nvConfig));
            if(ESP_OK != ret) {
                LOGC("Failed to update NVS data with error: (%s). Aborting...", esp_err_to_name(ret));
            }

            ret = handle->commit();
            if(ESP_OK != ret) {
                LOGC("Failed to commit NVS data with error: (%s). Aborting...", esp_err_to_name(ret));
            }
            
            LOGI("Device is unarmed after script execution.");
        }
    }
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
        Utils::delay(1000);
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
