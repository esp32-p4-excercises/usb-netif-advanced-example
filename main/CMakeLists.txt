idf_component_register(SRCS "main.cpp" "server.cpp" "ota.cpp"
                    INCLUDE_DIRS "." "include")


set(data ../data)
fatfs_create_spiflash_image(storage ${data} FLASH_IN_PROJECT)
