#include "mdns.h"

#include "MDNSResponder.hpp"
#include "Logger.hpp"

MDNSResponder::MDNSResponder(std::string hostname)
:hostname(hostname){}

ErrorCode MDNSResponder::start() {
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        LOGE("Failed to initialize mDNS service with code: %d", err);
        return ErrorCode::GeneralError;
    }

    //set hostname
    err = mdns_hostname_set(hostname.c_str());
    if(err) {
        LOGE("Failed to set mDNS hostname with code: %d", err);
        return ErrorCode::GeneralError;
    }

    LOGI("mDNS initialized succesfully. Hostname: %s.local", hostname.c_str());
    return ErrorCode::Success;
}