file(GLOB files "*.html" "*.js" "*.css" "*.png" "*.jpg" "*.gif" "*.svg")

foreach(file_path ${files})
    get_filename_component(file ${file_path} NAME)
    message("Adding target: ${file}_object")
    add_custom_target(
                        ${file}_object
                        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${file}.obj
                        VERBATIM)

    add_custom_command(
                        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file}.obj
                        COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-xtensa-le --binary-architecture xtensa ${CMAKE_CURRENT_LIST_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file}.obj
                        COMMENT Coverts ${file} to object file)
endforeach()