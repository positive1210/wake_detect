idf_component_register(SRCS "local_tts.c" "drugs_action.c" "esp_sr.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_board player 
                   )

component_compile_options(-w -Wno-format)