idf_component_register(SRCS "src/main.cpp" "src/WiFiAccessPoint.cpp" "src/Logger.cpp" "src/HttpServer.cpp"
                    PRIV_REQUIRES esp_wifi spi_flash nvs_flash esp_http_server
                    INCLUDE_DIRS "inc")
