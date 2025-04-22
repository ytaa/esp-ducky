#pragma once

#include "nvs_handle.hpp"

#include "WiFiAccessPoint.hpp"
#include "HttpServer.hpp"
#include "MdnsResponder.hpp"
#include "UsbDevice.hpp"
#include "Utils.hpp"

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

    struct NvConfig 
    {
        ArmingState armingState;
        UsbDeviceType usbDeviceType;
    };

    NvConfig nvConfig;
    WiFiAccessPoint ap;
    MdnsResponder mdns;
    HttpServer http;
    UsbDevice usb;

    void handleNvConfig(nvs::NVSHandle *handle);
    void handleNvScript(nvs::NVSHandle *handle);

public:
    EspDucky();
    ~EspDucky() = default;

    ErrorCode init();
    void run();

    ErrorCode handleScriptEndpoint(const std::string &request, std::string &response);
};
