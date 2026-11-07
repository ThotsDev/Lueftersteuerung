cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED ENV{IDF_PATH} OR NOT DEFINED ENV{ESP_MATTER_PATH})
    message(FATAL_ERROR "Source ESP-IDF and ESP-Matter export.sh first.")
endif()
set(PROJECT_VER "0.1.0")
set(PROJECT_VER_NUMBER 1)
set(ESP_MATTER_PATH "$ENV{ESP_MATTER_PATH}")
set(MATTER_SDK_PATH "${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip")
include("$ENV{IDF_PATH}/tools/cmake/project.cmake")
set(EXTRA_COMPONENT_DIRS
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components")
project(ventilation_controller)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-DCHIP_HAVE_CONFIG_H" APPEND)
