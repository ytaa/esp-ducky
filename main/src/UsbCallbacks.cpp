#include "UsbDevice.hpp"

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance){
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    UsbDevice* usbDevice = UsbDevice::getInstance(0);
    if (nullptr == usbDevice) {
        return nullptr;
    }
    
    return usbDevice->getReportDescriptor();
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
