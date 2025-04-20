#include <algorithm>

#include "HttpServer.hpp"
#include "Logger.hpp"

HttpServer::HttpServer(std::unordered_map<std::string, StaticEndpoint> &&staticEndpoints, 
    std::unordered_map<std::string, std::function<ErrorCode(const std::string&, std::string&)>> &&dynamicEndpointCallbacks): 
server(NULL), 
staticEndpoints(std::move(staticEndpoints)),
dynamicEndpointCallbacks(std::move(dynamicEndpointCallbacks))
{}

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
        httpd_uri_t uriHandler = {
            .uri=uri.c_str(), 
            .method=HTTP_GET, 
            .handler=handleStaticEndpoint, 
            .user_ctx=this
        };
        esp_err_t err = httpd_register_uri_handler(server, &uriHandler);
        if(err) {
            LOGE("Failed to register URI GET handler for '%s' with code: '%d'", uri.c_str(), err);
        }
    }

    /* Register URI handlers for dynamic endpoints */
    for (const auto & [uri, callback] : dynamicEndpointCallbacks) {
        httpd_uri_t uriHandler = {
            .uri=uri.c_str(), 
            .method=HTTP_GET, 
            .handler=handleDynamicEndpoint, 
            .user_ctx=this
        };
        esp_err_t err = httpd_register_uri_handler(server, &uriHandler);
        if(err) {
            LOGE("Failed to register URI GET handler for '%s' with code: '%d'", uri.c_str(), err);
        }

        uriHandler.method = HTTP_POST;
        err = httpd_register_uri_handler(server, &uriHandler);
        if(err) {
            LOGE("Failed to register URI POST handler for '%s' with code: '%d'", uri.c_str(), err);
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

esp_err_t HttpServer::handleStaticEndpoint(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
    const StaticEndpoint &endpoint = httpServer->staticEndpoints.at(req->uri);
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, endpoint.respBuf, endpoint.respLen);
    return ESP_OK;
}

esp_err_t HttpServer::handleDynamicEndpoint(httpd_req_t *req)
{
    HttpServer *httpServer = (HttpServer *)req->user_ctx;
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

    /* Call the dynamic endpoint callback */
    auto it = httpServer->dynamicEndpointCallbacks.find(req->uri);
    if(it != httpServer->dynamicEndpointCallbacks.end()) {
        auto callback = it->second;
        std::string response{};
        ErrorCode err = callback(reqBuf.get(), response);
        if(err != ErrorCode::Success) {
            LOGE("Failed to handle dynamic endpoint '%s' with error: %d", req->uri, err);
            return ESP_FAIL;
        }
        
        if(!response.empty()) {
            httpd_resp_sendstr(req, response.c_str());
        } 
    }

    return ESP_OK;
}