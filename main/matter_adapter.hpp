#pragma once
#include <esp_matter.h>
#include "ventilation.hpp"

// Called once before starting the Matter stack.
esp_err_t create_ventilation_endpoint(esp_matter::node_t *node,
                                      ventilation::Controller &controller);
esp_err_t ventilation_attribute_callback(esp_matter::attribute::callback_type_t type,
    uint16_t endpoint, uint32_t cluster, uint32_t attribute,
    esp_matter_attr_val_t *value, void *private_data);
