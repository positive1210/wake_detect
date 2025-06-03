set(srcs
    ./esp_tts_wav/wav_encoder.c
    ./esp_tts_wav/wav_decoder.c
    )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS
        "."
        "./esp_tts_wav"
    REQUIRES esp_board)

