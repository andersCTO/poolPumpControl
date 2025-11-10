idf_component_register(SRCS "app.c"
                       INCLUDE_DIRS "include"
                       REQUIRES pump_driver networking price_client scheduler storage sensors)
