#include "HttpServer.hpp"
#include "Logger.hpp"

HttpServer::HttpServer(): 
server(NULL), 
uri_get_index({
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = cbk_get_index,
    .user_ctx = this
}) {}

ErrorCode HttpServer::start(){
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (ESP_OK != httpd_start(&server, &config)) {
        return ErrorCode::GeneralError;

        LOGE("Failed to start HTTP Server");
    }

    /* Register URI handlers */
    httpd_register_uri_handler(server, &uri_get_index);

    LOGI("HTTP Server started");

    return ErrorCode::Success;
}

void HttpServer::stop(){
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

esp_err_t HttpServer::cbk_get_index(httpd_req_t *req)
{
    /* Send a simple response */
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}