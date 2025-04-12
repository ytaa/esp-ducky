idf_component_register(SRCS "src/main.cpp" "src/WiFiAccessPoint.cpp" "src/Logger.cpp"
                    PRIV_REQUIRES esp_wifi spi_flash nvs_flash
                    INCLUDE_DIRS "inc")
