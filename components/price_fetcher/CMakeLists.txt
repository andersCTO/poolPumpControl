idf_component_register(SRCS "price_fetcher.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_client json main)
