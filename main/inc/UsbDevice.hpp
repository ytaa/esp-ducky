#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>

#include "tinyusb.h"
#include "class/hid/hid_device.h"

#include "Utils.hpp"

#define USB_HID_DESCRIPTOR_NUM 146
#define USB_STRING_DESCRIPTOR_NUM 5
#define USB_CONFIG_DESCRIPTOR_NUM 34

class UsbDevice
{
private:
    bool isStartedFlag;
public:

    UsbDevice();
    virtual ~UsbDevice() = default;

    ErrorCode start();
    ErrorCode stop();

    bool isStarted();
    bool isMounted();

    ErrorCode enableJTAG();

    void hidKeyboardReport(std::initializer_list<uint8_t> keysList, uint8_t modifier = 0u);
    void hidKeyStroke(std::initializer_list<uint8_t> keysList, uint8_t modifier = 0u, uint32_t delay = 50u);
    void hidKeyWrite(std::string text, uint8_t modifier = 0u, uint32_t delay = 50u);
};
