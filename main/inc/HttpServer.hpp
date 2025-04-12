#pragma once

#include "esp_http_server.h"

#include "Utils.hpp"

class HttpServer {
public:
    HttpServer();
    ~HttpServer() = default;

    ErrorCode start();
    void stop();

    static esp_err_t cbk_get_index(httpd_req_t *req);
private:
    httpd_handle_t server;
    httpd_uri_t uri_get_index;
};