#include <string>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

class WiFiAccessPoint
{
private:
    std::string ssid;       // SSID of the access point
    std::string password;   // Password for the access point
    std::uint8_t channel;        // Channel number (1-13 for 2.4GHz, 36-165 for 5GHz)
    std::uint8_t maxConnections; // Maximum number of connections
public:
    WiFiAccessPoint(const std::string& ssid, const std::string& password, std::uint8_t channel = 1u, std::uint8_t maxConnections = 5u);
    void start();
    void stop();
    static void eventHandler(void* arg, esp_event_base_t eventBase, std::int32_t eventId, void* eventData);
};