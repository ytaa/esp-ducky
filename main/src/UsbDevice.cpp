#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "UsbDevice.hpp"
#include "Logger.hpp"

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

static const char* hid_string_descriptor[] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
};

static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

UsbDevice::UsbDevice() :
isStartedFlag(false)
{}

ErrorCode UsbDevice::start() {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
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

bool UsbDevice::isStarted() {
    return isStartedFlag;
}

bool UsbDevice::isMounted() {
    return tud_mounted();
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

void UsbDevice::hidKeyboardReport(std::initializer_list<uint8_t> keysList, uint8_t modifier) {
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

void UsbDevice::hidKeyStroke(std::initializer_list<uint8_t> keysList, uint8_t modifier, uint32_t delay) {
    hidKeyboardReport(keysList, modifier);
    vTaskDelay(delay / portTICK_PERIOD_MS);
    hidKeyboardReport({0}, 0);
    vTaskDelay(delay / portTICK_PERIOD_MS);
}

void UsbDevice::hidKeyWrite(std::string text, uint8_t modifier, uint32_t delay){
    for (unsigned char chr : text) {
        if (chr < 128) {
            uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };
            if (conv_table[chr][0]) modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
            hidKeyStroke({conv_table[chr][1]}, modifier, delay);
        }
    }
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance){
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    (void) instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen){
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize){
    return;
}
