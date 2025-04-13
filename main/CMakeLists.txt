if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set(WEB_FILES "index.html" "script.js")
    foreach(file ${WEB_FILES})
        message("Creating target for web file: ${file}")
        set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/${file}.obj")
        list(APPEND WEB_FILES_OBJ "${OBJ_FILE}")
        add_custom_target(${file}_object 
                            DEPENDS ${OBJ_FILE}
                            VERBATIM)

        add_custom_command(OUTPUT ${OBJ_FILE}
                            COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-xtensa-le --binary-architecture xtensa ${CMAKE_CURRENT_LIST_DIR}/web/${file} ${OBJ_FILE}
                            COMMENT Coverts ${file} to object file)
    endforeach()
endif()

idf_component_register(SRCS "src/main.cpp" "src/WiFiAccessPoint.cpp" "src/Logger.cpp" "src/HttpServer.cpp" "src/MDNSResponder.cpp" ${WEB_FILES_OBJ}
                       PRIV_REQUIRES esp_wifi spi_flash nvs_flash esp_http_server
                       INCLUDE_DIRS "inc"
                       WHOLE_ARCHIVE)

file(GLOB files "web/*.html" "web/*.js" "web/*.css" "web/*.png" "web/*.jpg" "web/*.gif" "web/*.svg")

