#pragma once

#include "WiFiAccessPoint.hpp"
#include "HttpServer.hpp"
#include "MdnsResponder.hpp"
#include "UsbDevice.hpp"
#include "Utils.hpp"

class EspDucky
{
private:
    WiFiAccessPoint ap;
    MdnsResponder mdns;
    HttpServer http;
    UsbDevice usb;
public:
    EspDucky();
    ~EspDucky() = default;

    ErrorCode init();
    void run();

    ErrorCode handleScriptEndpoint(const std::string &request, std::string &response);
};
