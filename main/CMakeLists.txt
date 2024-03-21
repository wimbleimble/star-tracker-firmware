idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES web_server a4988_driver esp_wifi nvs_flash)
