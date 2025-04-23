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

    using EndpointCallback = std::function<ErrorCode(httpd_req_t &, const std::string&, std::string&)>;
    struct StaticEndpoint {
        const char *respBuf;
        size_t respLen;
        const std::string_view mime;
    };

    struct DynamicEndpoint {
        EndpointCallback callback;
        const std::string_view mime;
    };

    explicit HttpServer(std::unordered_map<std::string, StaticEndpoint> &&staticEndpoints, 
        std::unordered_map<std::string, DynamicEndpoint> &&dynamicEndpoints);
    ~HttpServer() = default;

    ErrorCode start();
    void stop();

    static esp_err_t handleStaticEndpoint(httpd_req_t *req);
    static esp_err_t handleDynamicEndpoint(httpd_req_t *req);

private:
    httpd_handle_t server;
    const std::unordered_map<std::string, StaticEndpoint> staticEndpoints;
    const std::unordered_map<std::string, DynamicEndpoint> dynamicEndpoints;
};
