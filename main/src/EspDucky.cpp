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
#include "StaticWebData.hpp"

#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default

EspDucky::EspDucky() :
nvConfig(ArmingState::Unarmed, UsbDeviceType::Hid), 
nvScript(std::nullopt),
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
},std::unordered_map<std::string, HttpServer::DynamicEndpoint>{
    {"/script", {
            .callback = [this](httpd_req_t &http, const std::string &request, std::string &response) {
                return handleScriptEndpoint(http, request, response);
            },
            .mime = "application/json"
        }
    },
    {"/config", {
        .callback = [this](httpd_req_t &http, const std::string &request, std::string &response) {
            return handleConfigEndpoint(http, request, response);
        },
        .mime = "application/json"
    }
}
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
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READWRITE, &ret);
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
    esp_err_t ret = handle->get_blob(NVS_NV_CONFIG_KEY, &nvConfig, sizeof(nvConfig));
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
    LOGI("Reading script from NVS...");
    
    // Read script size from NVS
    uint32_t nvScriptSize = 0u;
    std::unique_ptr<uint8_t[]> nvScriptData{nullptr};

    esp_err_t ret = handle->get_item(NVS_NV_SCRIPT_SIZE_KEY, nvScriptSize);
    if(ESP_OK == ret)
    {
        // Read script data from NVS
        nvScriptData = std::make_unique<uint8_t[]>(nvScriptSize);
        ret = handle->get_blob(NVS_NV_SCRIPT_DATA_KEY, nvScriptData.get(), nvScriptSize);
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
        nvScript = Script::deserialize(std::span<const uint8_t>(nvScriptData.get(), nvScriptSize));
        if (!nvScript) {
            LOGE("Failed to deserialize script from NVS. The armed state is ignored.");
            return;
        }
        else {

            if(nvConfig.armingState == ArmingState::Unarmed) {
                LOGI("Device is unarmed. No script is executed at startup.");
                return;
            }
        
            if(UsbDeviceType::Hid != nvConfig.usbDeviceType && UsbDeviceType::HidMsd != nvConfig.usbDeviceType) {
                LOGW("Device is not in HID mode. Script execution is not possible.");
                return;
            }

            // The device is armed, HID is enabled and the script is successfully deserialized
            if(!waitForUsbMount()) {
                LOGW("USB device not mounted during startup. The armed state is ignored.");
                return;
            }

            LOGD("Running deserialized script from NVS:\n%s", nvScript->toString().c_str());

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
            ret = handle->set_blob(NVS_NV_CONFIG_KEY, &nvConfig, sizeof(nvConfig));
            if(ESP_OK != ret) {
                LOGC("Failed to update NVS data with error: (%s). Aborting...", esp_err_to_name(ret));
            }

            ret = handle->commit();
            if(ESP_OK != ret) {
                LOGC("Failed to commit NVS data with error: (%s). Aborting...", esp_err_to_name(ret));
            }
            
            LOGI("Device is unarmed after script execution. Config stored in NVS.");
        }
    }
}

bool EspDucky::waitForUsbMount(uint32_t timeoutMs) {
    uint32_t elapsedTime = 0u;

    LOGI("Waiting for USB device to mount... (%d ms)", timeoutMs);

    while(!usb.isMounted() && (elapsedTime < timeoutMs)) {
        Utils::delay(100u);
        elapsedTime += 100u;
    }

    if(!usb.isMounted()) {
        LOGW("USB device not mounted after %d ms", timeoutMs);
        return false;
    }

    LOGI("USB device mounted after ~%d ms", elapsedTime);

    return true;
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

ErrorCode EspDucky::handleScriptEndpoint(httpd_req_t &http, const std::string &request, std::string &response) {
    switch(http.method) {
        case HTTP_GET: {
            return handleScriptEndpointGet(http, request, response);
        }
        case HTTP_POST: {
            return handleScriptEndpointPost(http, request, response);
        }
        default: {
            LOGE("Unsupported HTTP method: %d", http.method);
        }
    }

    return ErrorCode::InvalidArgument;
}

ErrorCode EspDucky::handleScriptEndpointGet(httpd_req_t &http, const std::string &request, std::string &response) {
    cJSON *respJson = cJSON_CreateObject();
    if (!respJson) {
        LOGE("Failed to create JSON response object");
        return ErrorCode::GeneralError;
    }

    // Ignore return value - the functions return pointer to the root object
    (void)cJSON_AddStringToObject(respJson, "script", nvScript ? nvScript->toString().c_str() : "REM No script stored in the device\nREM Write your script here and press Run or Save button\n");

    char *respJsonStr = cJSON_PrintUnformatted(respJson);
    if( !respJsonStr) {
        LOGE("Failed to create JSON string from response object");
        cJSON_Delete(respJson); // Free the response json object
        return ErrorCode::GeneralError;
    }

    response = respJsonStr;

    std::free(respJsonStr); // Free the JSON string
    cJSON_Delete(respJson); // Free the response json object

    return ErrorCode::Success;
}

ErrorCode EspDucky::handleScriptEndpointPost(httpd_req_t &http, const std::string &request, std::string &response) {
    cJSON *reqJson = cJSON_Parse(request.c_str());
    if (!reqJson) {
        LOGE("Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return ErrorCode::InvalidArgument;
    }

    cJSON *scriptJson = cJSON_GetObjectItemCaseSensitive(reqJson, "script");
    if (!cJSON_IsString(scriptJson) || (scriptJson->valuestring == NULL)) {
        LOGE("Invalid JSON format: 'script' is not a string");
        cJSON_Delete(reqJson);
        return ErrorCode::InvalidArgument;
    }

    cJSON *actionJson = cJSON_GetObjectItemCaseSensitive(reqJson, "action");
    if (!cJSON_IsNumber(actionJson)) {
        LOGE("Invalid JSON format: 'action' is not a number");
        cJSON_Delete(reqJson);
        return ErrorCode::InvalidArgument;
    }

    LOGD("Request script: '%s'", scriptJson->valuestring);
    LOGD("Request action: '%d'", actionJson->valueint);

    ScriptEndpointAction action = static_cast<ScriptEndpointAction>(actionJson->valueint);

    auto script = Script::parse(scriptJson->valuestring);
    cJSON_Delete(reqJson); // Free the request json object

    if (!script) {
        LOGE("Failed to parse script: %s", scriptJson->valuestring);
        return ErrorCode::GeneralError;
    }

    LOGD("Script parsing successful:\n%s", script->toString().c_str());

    switch (action) {
        case ScriptEndpointAction::Run: {
            if (scriptRun(*script) != ErrorCode::Success) {
                LOGE("Failed to run script");
                return ErrorCode::GeneralError;
            }
            break;
        }
        case ScriptEndpointAction::Save: {
            if (scriptSave(*script) != ErrorCode::Success) {
                LOGE("Failed to save script");
                return ErrorCode::GeneralError;
            }
            break;
        }
        default: {
            LOGE("Invalid action: %d", action);
            return ErrorCode::InvalidArgument;
        }
    }


    // Prepare response
    cJSON *respJson = cJSON_CreateObject();
    if (!respJson) {
        LOGE("Failed to create JSON response object");
        return ErrorCode::GeneralError;
    }

    cJSON_AddStringToObject(respJson, "status", "success");

    char *respJsonStr = cJSON_PrintUnformatted(respJson);
    if (!respJsonStr) {
        LOGE("Failed to create JSON string from response object");
        cJSON_Delete(respJson); // Free the response json object
        return ErrorCode::GeneralError;
    }

    response = respJsonStr;

    std::free(respJsonStr); // Free the JSON string
    cJSON_Delete(respJson); // Free the response json object

    return ErrorCode::Success;    
}

ErrorCode EspDucky::scriptRun(Script &script) {
    if(usb.isMounted()) {
        LOGD("Starting script execution...");

        if (script.run(usb) != ErrorCode::Success) {
            LOGE("Failed to run script");
            return ErrorCode::GeneralError;
        }

        LOGI("Script executed successfully");
    }
    else
    {
        LOGW("USB device not mounted. Skipping script execution.");
    }

    return ErrorCode::Success;
}

ErrorCode EspDucky::scriptSave(Script &script) {
    auto serializedScript = script.serialize();
    if (serializedScript.empty()) {
        LOGE("Failed to serialize script");
        return ErrorCode::GeneralError;
    }

    // Open NVS handle
    esp_err_t ret = 0;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READWRITE, &ret);
    if(ESP_OK != ret) {
        LOGE("Failed to open NVS handle with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }
    
    // Write script size to NVS
    uint32_t scriptSize = static_cast<uint32_t>(serializedScript.size());
    ret = handle->set_item(NVS_NV_SCRIPT_SIZE_KEY, scriptSize);
    if(ESP_OK != ret) {
        LOGE("Failed to update nvScriptSize with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }

    // Write script data to NVS
    ret = handle->set_blob(NVS_NV_SCRIPT_DATA_KEY, serializedScript.data(), scriptSize);
    if(ESP_OK != ret) {
        LOGE("Failed to update nvScriptData with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }

    ret = handle->commit();
    if(ESP_OK != ret) {
        LOGE("Failed to commit NVS data with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }

    // Store the script in nvScript variable
    nvScript = std::move(script); 

    LOGI("New script successfully stored in NVS");

    return ErrorCode::Success;
}

ErrorCode EspDucky::handleConfigEndpoint(httpd_req_t &http, const std::string &request, std::string &response) {
    switch(http.method) {
        case HTTP_GET: {
            return handleConfigEndpointGet(http, request, response);
        }
        case HTTP_POST: {
            return handleConfigEndpointPost(http, request, response);
        }
        default: {
            LOGE("Unsupported HTTP method: %d", http.method);
        }
    }

    return ErrorCode::InvalidArgument;
}

ErrorCode EspDucky::handleConfigEndpointGet(httpd_req_t &http, const std::string &request, std::string &response) {
    cJSON *respJson = cJSON_CreateObject();
    if (!respJson) {
        LOGE("Failed to create JSON response object");
        return ErrorCode::GeneralError;
    }

    // Ignore return value - the functions return pointer to the root object
    (void)cJSON_AddNumberToObject(respJson, "armingState", static_cast<int>(nvConfig.armingState));
    (void)cJSON_AddNumberToObject(respJson, "usbDeviceType", static_cast<int>(nvConfig.usbDeviceType));

    char *respJsonStr = cJSON_PrintUnformatted(respJson);
    if( !respJsonStr) {
        LOGE("Failed to create JSON string from response object");
        cJSON_Delete(respJson); // Free the response json object
        return ErrorCode::GeneralError;
    }

    response = respJsonStr;

    std::free(respJsonStr); // Free the JSON string
    cJSON_Delete(respJson); // Free the response json object

    return ErrorCode::Success;
}

ErrorCode EspDucky::handleConfigEndpointPost(httpd_req_t &http, const std::string &request, std::string &response) {
    cJSON *reqJson = cJSON_Parse(request.c_str());
    if (!reqJson) {
        LOGE("Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return ErrorCode::InvalidArgument;
    }

    cJSON *armingStateJson = cJSON_GetObjectItemCaseSensitive(reqJson, "armingState");
    if (!cJSON_IsNumber(armingStateJson)) {
        LOGE("Invalid JSON format: 'armingState' is not a number");
        cJSON_Delete(reqJson);
        return ErrorCode::InvalidArgument;
    }

    cJSON *usbDeviceTypeJson = cJSON_GetObjectItemCaseSensitive(reqJson, "usbDeviceType");
    if (!cJSON_IsNumber(usbDeviceTypeJson)) {
        LOGE("Invalid JSON format: 'usbDeviceType' is not a number");
        cJSON_Delete(reqJson);
        return ErrorCode::InvalidArgument;
    }

    LOGD("Request armingState: '%d'", armingStateJson->valueint);
    LOGD("Request usbDeviceType: '%d'", usbDeviceTypeJson->valueint);

    nvConfig.armingState = static_cast<ArmingState>(armingStateJson->valueint);
    nvConfig.usbDeviceType = static_cast<UsbDeviceType>(usbDeviceTypeJson->valueint);

    cJSON_Delete(reqJson); // Free the request json object

    // Open NVS handle
    esp_err_t ret = 0;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READWRITE, &ret);
    if(ESP_OK != ret) {
        LOGE("Failed to open NVS handle with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }
    
    // Write nvConfig to NVS
    ret = handle->set_blob(NVS_NV_CONFIG_KEY, &nvConfig, sizeof(nvConfig));
    if(ESP_OK != ret) {
        LOGE("Failed to update NVS data with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }

    ret = handle->commit();
    if(ESP_OK != ret) {
        LOGE("Failed to commit NVS data with error: (%s)", esp_err_to_name(ret));
        return ErrorCode::GeneralError;
    }

    LOGI("New configuration successfully stored in NVS");

    // Prepare response
    cJSON *respJson = cJSON_CreateObject();
    if (!respJson) {
        LOGE("Failed to create JSON response object");
        return ErrorCode::GeneralError;
    }

    cJSON_AddStringToObject(respJson, "status", "success");

    char *respJsonStr = cJSON_PrintUnformatted(respJson);
    if (!respJsonStr) {
        LOGE("Failed to create JSON string from response object");
        cJSON_Delete(respJson); // Free the response json object
        return ErrorCode::GeneralError;
    }

    response = respJsonStr;

    std::free(respJsonStr); // Free the JSON string
    cJSON_Delete(respJson); // Free the response json object

    return ErrorCode::Success;
}