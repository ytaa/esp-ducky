#include "HttpServer.hpp"
#include "Logger.hpp"

extern char _binary_C__w_esp_ducky_main_web_index_html_start;
extern char _binary_C__w_esp_ducky_main_web_index_html_end;
extern char _binary_C__w_esp_ducky_main_web_script_js_start;
extern char _binary_C__w_esp_ducky_main_web_script_js_end;

HttpServer::HttpServer(): 
server(NULL), 
uriHandlers(
{
    {"/", {
            .uri = {.uri="/", .method=HTTP_GET, .handler=cbk_handle_uri, .user_ctx=this}, 
            .respBuf = (const char*)&_binary_C__w_esp_ducky_main_web_index_html_start, 
            .respLen = (size_t)(&_binary_C__w_esp_ducky_main_web_index_html_end - &_binary_C__w_esp_ducky_main_web_index_html_start)
          }
    },
    {"/script.js", {
        .uri = {.uri="/script.js", .method=HTTP_GET, .handler=cbk_handle_uri, .user_ctx=this}, 
        .respBuf = (const char*)&_binary_C__w_esp_ducky_main_web_script_js_start, 
        .respLen = (size_t)(&_binary_C__w_esp_ducky_main_web_script_js_end - &_binary_C__w_esp_ducky_main_web_script_js_start)
      }
}
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
    for (const auto & [uri, handler] : uriHandlers) {
        esp_err_t err = httpd_register_uri_handler(server, &handler.uri);
        if(err) {
            LOGE("Failed to register URI handler for %s with code: %d", uri.c_str(), err);
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

esp_err_t HttpServer::cbk_handle_uri(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    const UriHandler &handler = httpServer->uriHandlers.at(req->uri);
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, handler.respBuf, handler.respLen);
    return ESP_OK;
}
