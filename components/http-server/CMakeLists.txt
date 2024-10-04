idf_component_register(SRCS "http-server.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_http_server esp_https_server
                    )
