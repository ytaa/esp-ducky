#include <algorithm>
#include <memory>

#include "HttpServer.hpp"
#include "Logger.hpp"

extern char _binary_C__w_esp_ducky_main_web_index_html_start;
extern char _binary_C__w_esp_ducky_main_web_index_html_end;
extern char _binary_C__w_esp_ducky_main_web_script_js_start;
extern char _binary_C__w_esp_ducky_main_web_script_js_end;

HttpServer::HttpServer(): 
server(NULL), 
staticEndpoints({
    {"/", {
            .uriHandler = {.uri="/", .method=HTTP_GET, .handler=cbk_handle_static_get, .user_ctx=this}, 
            .respBuf = (const char*)&_binary_C__w_esp_ducky_main_web_index_html_start, 
            .respLen = (size_t)(&_binary_C__w_esp_ducky_main_web_index_html_end - &_binary_C__w_esp_ducky_main_web_index_html_start)
        }
    },
    {"/script.js", {
            .uriHandler = {.uri="/script.js", .method=HTTP_GET, .handler=cbk_handle_static_get, .user_ctx=this}, 
            .respBuf = (const char*)&_binary_C__w_esp_ducky_main_web_script_js_start, 
            .respLen = (size_t)(&_binary_C__w_esp_ducky_main_web_script_js_end - &_binary_C__w_esp_ducky_main_web_script_js_start)
        }
    }
}),
dynamicEndpoints({{
    {.uri="/script", .method=HTTP_GET, .handler=cbk_handle_get_script, .user_ctx=this},
    {.uri="/script", .method=HTTP_POST, .handler=cbk_handle_post_script, .user_ctx=this},
    {.uri="/config", .method=HTTP_GET, .handler=cbk_handle_get_config, .user_ctx=this},
    {.uri="/config", .method=HTTP_POST, .handler=cbk_handle_post_config, .user_ctx=this}
}}) {}

ErrorCode HttpServer::start(){
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    server = NULL;

    /* Start the httpd server */
    if (ESP_OK != httpd_start(&server, &config) || !server) {
        LOGE("Failed to start HTTP Server");
        return ErrorCode::GeneralError;
    }

    /* Register URI handlers for static endpoints */
    for (const auto & [uri, endpoint] : staticEndpoints) {
        esp_err_t err = httpd_register_uri_handler(server, &endpoint.uriHandler);
        if(err) {
            LOGE("Failed to register URI handler for '%s' with code: '%d'", uri.c_str(), err);
        }
    }

    /* Register URI handlers for dynamic endpoints */
    for (const auto &uriHandler : dynamicEndpoints) {
        esp_err_t err = httpd_register_uri_handler(server, &uriHandler);
        if(err) {
            LOGE("Failed to register URI handler for '%s' with code: '%d'", uriHandler.uri, err);
        }
    }

    LOGI("HTTP Server started");

    return ErrorCode::Success;
}

void HttpServer::stop(){
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

esp_err_t HttpServer::cbk_handle_static_get(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    const StaticEndpoint &endpoint = httpServer->staticEndpoints.at(req->uri);
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, endpoint.respBuf, endpoint.respLen);
    return ESP_OK;
}

esp_err_t HttpServer::cbk_handle_get_script(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    (void)httpServer; // Avoid unused variable warning
    LOGI("HTTP request received: GET /script");
    return ESP_FAIL;
}

esp_err_t HttpServer::cbk_handle_post_script(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    (void)httpServer; // Avoid unused variable warning
    LOGI("HTTP request received: POST /script");

    int ret, remaining = req->content_len;
    std::unique_ptr<char[]> reqBuf = std::make_unique<char[]>(req->content_len + 1); // Reserve additional byte to ensure null-termination
    reqBuf[remaining] = '\0'; // Null-terminate the buffer

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, reqBuf.get(), remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    LOGD("Request buffer content: '%s'", reqBuf.get());

    /* Send response */
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

esp_err_t HttpServer::cbk_handle_get_config(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    (void)httpServer; // Avoid unused variable warning
    LOGI("HTTP request received: GET /config");
    return ESP_FAIL;
}

esp_err_t HttpServer::cbk_handle_post_config(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    (void)httpServer; // Avoid unused variable warning
    LOGI("HTTP request received: POST /config");

    int ret, remaining = req->content_len;
    std::unique_ptr<char[]> reqBuf = std::make_unique<char[]>(req->content_len + 1); // Reserve additional byte to ensure null-termination
    reqBuf[remaining] = '\0'; // Null-terminate the buffer

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, reqBuf.get(), remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    LOGD("Request buffer content: '%s'", reqBuf.get());

    /* Send response */
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}