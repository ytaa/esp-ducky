#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

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
    std::vector<uint8_t> reportDescriptor;
    std::vector<const char *> stringDescriptor;
    std::vector<uint8_t> configurationDescriptor;
    static std::vector<UsbDevice*> instances;
public:

    UsbDevice();
    virtual ~UsbDevice();

    ErrorCode start();
    ErrorCode stop();

    bool isStarted() const;
    bool isMounted() const;

    const uint8_t *getReportDescriptor() const;

    ErrorCode enableJTAG();

    void hidSendKeyboardReport(const std::vector<uint8_t> &keysList, uint8_t modifier = 0u);
    void hidKeyStroke(const std::vector<uint8_t> &keysList, uint8_t modifier = 0u, uint32_t delay = 14u);

    static UsbDevice* getInstance(uint8_t instanceIdx);
};
