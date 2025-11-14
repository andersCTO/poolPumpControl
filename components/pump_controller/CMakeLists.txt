idf_component_register(SRCS "pump_controller.c"
                       INCLUDE_DIRS "include"
                       REQUIRES relay_control nvs_storage)
