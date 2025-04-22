#include <algorithm>

#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "UsbDevice.hpp"
#include "Logger.hpp"

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

std::vector<UsbDevice*> UsbDevice::instances{};

UsbDevice::UsbDevice() :
isStartedFlag(false),
reportDescriptor({
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
}),
stringDescriptor({
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
}),
configurationDescriptor({
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, reportDescriptor.size(), 0x81, 16, 10),
})
{
    instances.push_back(this);
}

UsbDevice::~UsbDevice() {
    if(isStartedFlag) {
        stop();
    }

    auto it = std::find(instances.begin(), instances.end(), this);
    if (it != instances.end()) {
        instances.erase(it);
    }
}

ErrorCode UsbDevice::start() {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = stringDescriptor.data(),
        .string_descriptor_count = static_cast<int>(stringDescriptor.size()),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = configurationDescriptor.data(),
#endif // TUD_OPT_HIGH_SPEED
        .self_powered = false,
        .vbus_monitor_io = 0
    };

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err) {
        LOGE("Failed to initialize TinyUSB driver with code: %d", err);
        return ErrorCode::GeneralError;
    }

    isStartedFlag = true;
    LOGI("TinyUSB driver installed successful");
    return ErrorCode::Success;
}

ErrorCode UsbDevice::stop(){
    esp_err_t err = tinyusb_driver_uninstall();
    if (err) {
        LOGE("Failed to uninstall TinyUSB driver with code: %d", err);
        return ErrorCode::GeneralError;
    }

    isStartedFlag = false;
    LOGI("TinyUSB driver uninstalled successfully");

    return ErrorCode::Success;
}

bool UsbDevice::isStarted() const{
    return isStartedFlag;
}

bool UsbDevice::isMounted() const{
    return tud_mounted();
}

const uint8_t *UsbDevice::getReportDescriptor() const{
    return reportDescriptor.data();
}

ErrorCode UsbDevice::enableJTAG(){
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    esp_err_t err = usb_serial_jtag_driver_install(&usb_serial_jtag_config);
    if (err) {
        LOGE("Failed to install USB Serial JTAG driver with code: %d", err);
        return ErrorCode::GeneralError;
    }

    LOGI("USB Serial JTAG driver installed successfully");
    return ErrorCode::Success;
}

void UsbDevice::hidSendKeyboardReport(const std::vector<uint8_t> &keysList, uint8_t modifier) {
    if(keysList.size() > 0 && keysList.size() <= 6) {
        uint8_t keycode[6] = {0};
        uint8_t i = 0;
        for (auto key : keysList) {
                keycode[i] = key;
                i++;
        }
        (void)tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
    } 
    else {
        (void)tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, NULL);
    }
}

void UsbDevice::hidKeyStroke(const std::vector<uint8_t> &keysList, uint8_t modifier, uint32_t delay) {
    hidSendKeyboardReport(keysList, modifier);
    Utils::delay(delay);
    (void)tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
    Utils::delay(delay);
}

UsbDevice* UsbDevice::getInstance(uint8_t instanceIdx)
{
    if(instanceIdx < instances.size()) {
        return instances[instanceIdx];
    } else {
        return nullptr;
    }
}
