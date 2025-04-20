if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set(WEB_FILES "index.html" "script.js")
    foreach(file ${WEB_FILES})
        message("Creating target for web file: ${file}")
        set(file_obj "${CMAKE_CURRENT_BINARY_DIR}/${file}.obj")
        list(APPEND WEB_FILES_OBJ "${file_obj}")
        add_custom_target(${file}_object 
                            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/web/${file}
                            VERBATIM)

        add_custom_command(OUTPUT ${file_obj}
                            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/web/${file}
                            COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-xtensa-le --binary-architecture xtensa ${CMAKE_CURRENT_LIST_DIR}/web/${file} ${file_obj}
                            COMMENT Coverts ${file} to object file)
    endforeach()
endif()

idf_component_register(SRCS "src/Main.cpp" "src/WiFiAccessPoint.cpp" "src/Logger.cpp" "src/HttpServer.cpp" "src/MdnsResponder.cpp" "src/UsbDevice.cpp" "src/UsbCallbacks.cpp" "src/Script.cpp" "src/EspDucky.cpp" ${WEB_FILES_OBJ}
                       PRIV_REQUIRES esp_wifi spi_flash nvs_flash esp_http_server esp_driver_gpio esp_driver_usb_serial_jtag json
                       INCLUDE_DIRS "inc"
                       WHOLE_ARCHIVE)
