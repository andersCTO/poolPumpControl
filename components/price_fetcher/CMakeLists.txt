idf_component_register(SRCS "price_fetcher.c" "price_refresh_task.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_client json main wifi_manager)
