#pragma once

#include <unordered_map>
#include <string>
#include <array>
#include <functional>
#include <memory>

#include "esp_http_server.h"

#include "Utils.hpp"

class HttpServer {
public:
    struct StaticEndpoint {
        const char *respBuf;
        size_t respLen;
    };

    explicit HttpServer(std::unordered_map<std::string, StaticEndpoint> &&staticEndpoints, 
        std::unordered_map<std::string, std::function<ErrorCode(const std::string&, std::string&)>> &&dynamicEndpointCallbacks);
    ~HttpServer() = default;

    ErrorCode start();
    void stop();

    static esp_err_t handleStaticEndpoint(httpd_req_t *req);
    static esp_err_t handleDynamicEndpoint(httpd_req_t *req);

private:
    httpd_handle_t server;
    const std::unordered_map<std::string, StaticEndpoint> staticEndpoints;
    const std::unordered_map<std::string, std::function<ErrorCode(const std::string&, std::string&)>> dynamicEndpointCallbacks;
};
