#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "tusb_msc_storage.h"

#include "Utils.hpp"

#define USB_HID_DESCRIPTOR_NUM 146
#define USB_STRING_DESCRIPTOR_NUM 5
#define USB_CONFIG_DESCRIPTOR_NUM 34

class UsbDevice
{
private:
    bool isStartedFlag;
    wl_handle_t wl_handle;
    uint8_t interfaceCount;
    uint8_t configurationDescriptorTotalLength;
    tusb_desc_device_t deviceDescriptor;
    std::vector<uint8_t> reportDescriptor;
    std::vector<const char *> stringDescriptor;
    std::vector<uint8_t> configurationDescriptor;
    static std::vector<UsbDevice*> instances;

    void enableHID();
    void enableMSC();
public:

    enum class DeviceClass : uint8_t
    {
        SerialJtag,
        Hid,
        Msc,
        HidMsc
    };

    UsbDevice();
    virtual ~UsbDevice();

    ErrorCode start(DeviceClass deviceClass);
    ErrorCode stop();

    bool isStarted() const;
    bool isMounted() const;

    const uint8_t *getReportDescriptor() const;

    ErrorCode enableJTAG();

    void hidSendKeyboardReport(const std::vector<uint8_t> &keysList, uint8_t modifier = 0u);
    void hidKeyStroke(const std::vector<uint8_t> &keysList, uint8_t modifier = 0u, uint32_t delay = 20u);

    static UsbDevice* getInstance(uint8_t instanceIdx);
};
