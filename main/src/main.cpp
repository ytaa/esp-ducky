#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"

#include "WiFiAccessPoint.hpp"
#include "Logger.hpp"
#include "HttpServer.hpp"
#include "MdnsResponder.hpp"
#include "UsbDevice.hpp"

#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default

extern "C" void app_main(void)
{
    LOGD("Initializing...");

    // Initialize BOOT button 
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = (gpio_pullup_t)true,
        .pull_down_en = (gpio_pulldown_t)false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&boot_button_config);
    if (ret) {
        LOGC("Failed to configure BOOT button with error: %d. Aborting...", ret);
    }

    //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        LOGW("Erasing NVS flash due to %s", ret == ESP_ERR_NVS_NO_FREE_PAGES ? "lack of free pages" : "new NVS version");
        ret = nvs_flash_erase();
        if(ret) {
            LOGC("Failed to erase NVS flash with error: %d. Aborting...", ret);
        }
        ret = nvs_flash_init();
    }
    if(ret) {
        LOGC("Failed to initialize NVS flash with error: %d. Aborting...", ret);
    }

    // Create an Access Point
    WiFiAccessPoint ap("esp-ducky", "ducky123");
    ap.start();

    MdnsResponder MdnsResponder("esp-ducky");
    MdnsResponder.start();

    HttpServer http{};
    http.start();

    UsbDevice usb{};
    usb.start();

    uint8_t usbDeviceDisableCountdown = 3;
    bool isHidDemoFinished = false;

    // Infinite loop to keep the task running
    for (unsigned int i = 0;; i++) {
        LOGD("Running for %d seconds", i);
        if(usb.isStarted()) {
            if(!gpio_get_level(APP_BUTTON)){
                if(usbDeviceDisableCountdown == 0) {
                    LOGI("Disabling USB Device");
                    usb.stop();
                    LOGI("Enabling serial JTAG");
                    usb.enableJTAG();
                }
                else{
                    LOGI("USB will be disabled in %d seconds", usbDeviceDisableCountdown);
                    usbDeviceDisableCountdown--;
                }
            } 
            else {
                usbDeviceDisableCountdown = 3;
                if(usb.isMounted()) {
                    if(!isHidDemoFinished) {
                        LOGI("Starting HID demo");

                        usb.hidKeyStroke({HID_KEY_GUI_LEFT, HID_KEY_R});
                        usb.hidKeyWrite("notepad.exe", 0, 25);
                        usb.hidKeyStroke({HID_KEY_ENTER}, 0, 500);
                        usb.hidKeyStroke({HID_KEY_CONTROL_LEFT, HID_KEY_N});
                        usb.hidKeyWrite("test", 0, 25);

                        isHidDemoFinished = true;
                        LOGI("HID demo finished");
                    }
                } 
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    LOGI("Restarting...");
    fflush(stdout);
    esp_restart();
}