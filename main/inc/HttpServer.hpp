#pragma once

#include "esp_http_server.h"

#include <unordered_map>
#include <string>

#include "Utils.hpp"

class HttpServer {
    struct StaticEndpoint {
        httpd_uri_t uriHandler;
        const char *respBuf;
        size_t respLen;
    };

public:
    HttpServer();
    ~HttpServer() = default;

    ErrorCode start();
    void stop();

    static esp_err_t cbk_handle_static_get(httpd_req_t *req);
    static esp_err_t cbk_handle_get_script(httpd_req_t *req);
    static esp_err_t cbk_handle_post_script(httpd_req_t *req);
    static esp_err_t cbk_handle_get_config(httpd_req_t *req);
    static esp_err_t cbk_handle_post_config(httpd_req_t *req);
private:
    constexpr static size_t DYNAMIC_ENDPOINTS_COUNT = 4u;

    httpd_handle_t server;
    const std::unordered_map<std::string, StaticEndpoint> staticEndpoints;
    const std::array<httpd_uri_t, DYNAMIC_ENDPOINTS_COUNT> dynamicEndpoints;
};
