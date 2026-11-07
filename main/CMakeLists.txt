idf_component_register(
    SRCS "app_main.cpp" "matter_adapter.cpp"
    INCLUDE_DIRS "." "../components/ventilation/include"
    REQUIRES esp_matter nvs_flash)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
