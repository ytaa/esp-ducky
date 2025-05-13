#include <algorithm>

#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_partition.h"

#include "UsbDevice.hpp"
#include "Logger.hpp"

std::vector<UsbDevice*> UsbDevice::instances{};

UsbDevice::UsbDevice() :
isStartedFlag(false),
wl_handle(WL_INVALID_HANDLE),
interfaceCount(0u),
configurationDescriptorTotalLength(0u),
deviceDescriptor({
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200u,
    .bDeviceClass = 0x00u,
    .bDeviceSubClass = 0x00u,
    .bDeviceProtocol = 0x00u,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0xDEADu, 
    .idProduct = 0xBEEFu,
    .bcdDevice = 0x0100u,
    .iManufacturer = 0x01u,
    .iProduct = 0x02u,
    .iSerialNumber = 0x03u,
    .bNumConfigurations = 0x01u
}),
reportDescriptor({
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
}),
stringDescriptor({
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "esp-ducky",           // 1: Manufacturer
    "esp-ducky",           // 2: Product
    "B16B00B5",            // 3: Serial Number 
    "Interface",           // 4: HID/MSC Interface
}),
configurationDescriptor({
    // Configuration number, interface count, string index, total length, attribute, power in mA
    //TUD_CONFIG_DESCRIPTOR(1, 2, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    //TUD_HID_DESCRIPTOR(0, 4, false, reportDescriptor.size(), 0x81, 16, 10),
    // Interface number, string index, EP Out & EP In address, EP size
    //TUD_MSC_DESCRIPTOR(1, 4, 0x01, 0x82, 64),
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

void UsbDevice::enableHID() {
    // add the HID interface descriptor to the configuration descriptor
    std::vector<uint8_t> hidDescriptor = {
        // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR(interfaceCount++, 4, false, reportDescriptor.size(), 0x81, 16, 10),
    };

    configurationDescriptor.insert(configurationDescriptor.end(), hidDescriptor.begin(), hidDescriptor.end());

    // add the length of the HID descriptor to the total length of the configuration descriptor
    configurationDescriptorTotalLength += TUD_HID_DESC_LEN;
}

void UsbDevice::enableMSC() {
    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
    
    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if(data_partition == NULL) {
        LOGE("Failed to find data partition");
        return;
    }

    if( wl_mount(data_partition, &wl_handle) != ESP_OK) {
        LOGE("Failed to mount data partition");
        return;
    }

    tinyusb_msc_spiflash_config_t config_spi = {
        .wl_handle = wl_handle,
        .callback_mount_changed = NULL,
        .callback_premount_changed = NULL,  
        .mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 0,
            .disk_status_check_enable = false,
            .use_one_fat = false
        }
    };

    if(tinyusb_msc_storage_init_spiflash(&config_spi) != ESP_OK) {
        LOGE("Failed to initialize TinyUSB storage");
        return;
    }

    if(tinyusb_msc_storage_mount("/data") != ESP_OK) {
        LOGE("Failed to mount TinyUSB storage");
        return;
    }

    // add the MSC interface descriptor to the configuration descriptor
    std::vector<uint8_t> mscDescriptor = {
        // Interface number, string index, EP Out & EP In address, EP size
        TUD_MSC_DESCRIPTOR(interfaceCount++, 4, 0x01, 0x82, 64)
    };

    configurationDescriptor.insert(configurationDescriptor.end(), mscDescriptor.begin(), mscDescriptor.end());

    // add the length of the MSC descriptor to the total length of the configuration descriptor
    configurationDescriptorTotalLength += TUD_MSC_DESC_LEN;
}

ErrorCode UsbDevice::start(UsbDevice::DeviceClass deviceClass) {

    switch(deviceClass) {
        case DeviceClass::SerialJtag:
            LOGI("Starting USB Serial JTAG device...");
            // No additional configuration needed for Serial JTAG
            return ErrorCode::Success;
        case DeviceClass::Hid:
            LOGI("Starting USB HID device...");
            enableHID();
            break;
        case DeviceClass::Msc:
            LOGI("Starting USB MSC device...");
            enableMSC();
            break;
        case DeviceClass::HidMsc:
            LOGI("Starting USB HID + MSC device...");
            enableHID();
            enableMSC();
            break;
        default:
            LOGE("Invalid USB device class: %d", static_cast<int>(deviceClass));
            return ErrorCode::InvalidArgument;
    }

    // Add the length of the configuration descriptor header to the total length of the configuration descriptor
    configurationDescriptorTotalLength += TUD_CONFIG_DESC_LEN;

    // Add the configuration descriptor header to the configuration descriptor
    std::vector<uint8_t> configurationDescriptorHeader = {
        // Configuration number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, interfaceCount, 0, configurationDescriptorTotalLength, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100)
    };
    configurationDescriptor.insert(configurationDescriptor.begin(), configurationDescriptorHeader.begin(), configurationDescriptorHeader.end());

    // Prepare TinyUSB configuration
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &deviceDescriptor,
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

    // Initialize TinyUSB driver
    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err) {
        LOGE("Failed to initialize TinyUSB driver with code: %d", err);
        return ErrorCode::GeneralError;
    }

    isStartedFlag = true;
    LOGI("USB device started successfully");
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
