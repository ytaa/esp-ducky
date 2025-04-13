#pragma once

#include "esp_http_server.h"

#include <unordered_map>
#include <string>

#include "Utils.hpp"

class HttpServer {
    struct UriHandler {
        httpd_uri_t uri;
        const char *respBuf;
        size_t respLen;
    };

public:
    HttpServer();
    ~HttpServer() = default;

    ErrorCode start();
    void stop();

    static esp_err_t cbk_handle_uri(httpd_req_t *req);
private:
    httpd_handle_t server;
    const std::unordered_map<std::string, UriHandler> uriHandlers;
};