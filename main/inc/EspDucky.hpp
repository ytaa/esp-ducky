#pragma once

#include "nvs_handle.hpp"

#include "WiFiAccessPoint.hpp"
#include "HttpServer.hpp"
#include "MdnsResponder.hpp"
#include "UsbDevice.hpp"
#include "Utils.hpp"
#include "Script.hpp"

class EspDucky
{
private:
    enum class ArmingState : uint8_t
    {
        Unarmed,
        SingleRun,
        Persistent
    };

    enum class UsbDeviceType : uint8_t
    {
        SerialJtag,
        Hid,
        Msd,
        HidMsd
    };

    enum class ScriptEndpointAction : uint8_t
    {
        Run,
        Save
    };

    struct NvConfig 
    {
        ArmingState armingState;
        UsbDeviceType usbDeviceType;
    };

    static constexpr const char *NVS_NAMESPACE = "esp-ducky";
    static constexpr const char *NVS_NV_CONFIG_KEY = "nvConfig";
    static constexpr const char *NVS_NV_SCRIPT_SIZE_KEY = "nvScriptSize";
    static constexpr const char *NVS_NV_SCRIPT_DATA_KEY = "nvScriptData";

    NvConfig nvConfig;
    std::optional<Script> nvScript;
    WiFiAccessPoint ap;
    MdnsResponder mdns;
    HttpServer http;
    UsbDevice usb;

    void handleNvConfig(nvs::NVSHandle *handle);
    void handleNvScript(nvs::NVSHandle *handle);
    bool waitForUsbMount(uint32_t timeoutMs = 5000u);

    ErrorCode handleScriptEndpoint(httpd_req_t &http, const std::string &request, std::string &response);
    ErrorCode handleScriptEndpointGet(httpd_req_t &http, const std::string &request, std::string &response);
    ErrorCode handleScriptEndpointPost(httpd_req_t &http, const std::string &request, std::string &response);

    ErrorCode scriptRun(Script &script);
    ErrorCode scriptSave(Script &script);
    
    ErrorCode handleConfigEndpoint(httpd_req_t &http, const std::string &request, std::string &response);
    ErrorCode handleConfigEndpointGet(httpd_req_t &http, const std::string &request, std::string &response);
    ErrorCode handleConfigEndpointPost(httpd_req_t &http, const std::string &request, std::string &response);

public:
    EspDucky();
    ~EspDucky() = default;

    ErrorCode init();
    void run();
};
