#include <cstring>

#include "WiFiAccessPoint.hpp"
#include "Logger.hpp"

WiFiAccessPoint::WiFiAccessPoint(const std::string& ssid, const std::string& password, std::uint8_t channel, std::uint8_t maxConnections)
: ssid(ssid), password(password), channel(channel), maxConnections(maxConnections) {}

void WiFiAccessPoint::start()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &eventHandler,
                                                        this,
                                                        NULL));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    strcpy((char*)wifi_config.ap.ssid, ssid.c_str());
    wifi_config.ap.ssid_len = ssid.length();
    wifi_config.ap.channel = channel;
    strcpy((char*)wifi_config.ap.password, password.c_str());
    wifi_config.ap.max_connection = maxConnections;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.pmf_cfg.required = true;

    if (password.length() == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    LOGI("WiFi AP started");
    LOGI("SSID: '%s', password: '%s', channel: '%d'", ssid.c_str(), password.c_str(), channel);
}

void WiFiAccessPoint::stop()
{

}

void WiFiAccessPoint::eventHandler(void* arg, esp_event_base_t eventBase, std::int32_t eventId, void* eventData)
{
    WiFiAccessPoint* ap = static_cast<WiFiAccessPoint*>(arg);
    if (eventId == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) eventData;
        LOGI("WiFi AP '%s' join - mac: '" MACSTR "', AID: '%d'",
            ap->ssid.c_str(), MAC2STR(event->mac), event->aid);
    } else if (eventId == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) eventData;
        LOGI("WiFi AP '%s' leave - mac: '" MACSTR "', AID: '%d', reason: '%d'",
            ap->ssid.c_str(), MAC2STR(event->mac), event->aid, event->reason);
    }
}