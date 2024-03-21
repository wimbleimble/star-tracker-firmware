# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(star_tracker)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/index.html
    COMMAND pug ${PROJECT_SOURCE_DIR}/web_front_end/index.pug -o ${PROJECT_BINARY_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/web_front_end/index.pug
    ${PROJECT_SOURCE_DIR}/web_front_end/index.js
    ${PROJECT_SOURCE_DIR}/web_front_end/style.css)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/index.html.gz
    COMMAND gzip -9 ${PROJECT_BINARY_DIR}/index.html -c > ${PROJECT_BINARY_DIR}/index.html.gz
    DEPENDS ${PROJECT_BINARY_DIR}/index.html)

target_add_binary_data(star_tracker.elf "${PROJECT_BINARY_DIR}/index.html.gz" BINARY DEPENDS ${PROJECT_BINARY_DIR}/index.html.gz)
