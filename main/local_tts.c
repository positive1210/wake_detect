#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_tts.h"
#include "esp_tts_voice_template.h"
#include "esp_tts_voice_xiaole.h"
#include "esp_tts_player.h"
#include "esp_board.h"
#include "wav_encoder.h"
#include "esp_partition.h"

// #define PROMPT_STRING_MAX_LEN 100
// static char prompt_string[PROMPT_STRING_MAX_LEN];

void audio_tts(char *prompt_word)
{
    /*** 1. create esp tts handle ***/
    // initial voice set from separate voice data partition
    const esp_partition_t* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "voice_data");
    if (part==NULL) { 
        printf("Couldn't find voice data partition!\n"); 
        return 0;
    } else {
        printf("voice_data paration size:%d\n", part->size);
    }
    void* voicedata;
    esp_partition_mmap_handle_t mmap;
    esp_err_t err=esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, &voicedata, &mmap);
    if (err != ESP_OK) {
        printf("Couldn't map voice data partition!\n"); 
        return 0;
    }
    esp_tts_voice_t *voice=esp_tts_voice_set_init(&esp_tts_voice_template, (int16_t*)voicedata); //初始化语音合成
    
    esp_tts_handle_t tts_handle=esp_tts_create(voice);
    /*** 2. play prompt text ***/
    // strcpy(prompt_string, prompt_word);
    char *prompt1="欢迎使用乐鑫语音合成";
    if (esp_tts_parse_chinese(tts_handle, prompt1)) {
            int len[1]={0};
            do {
                int16_t *pcm_data=esp_tts_stream_play(tts_handle, len, 3);
                esp_audio_play(pcm_data, len[0], 80);
            } while(len[0]>0);
    }
    esp_tts_stream_reset(tts_handle);
}

// void audio_tts_init()
// {
//     xTaskCreatePinnedToCore(&audio_tts_task, "audio_tts_task", 4*1024, (void*)prompt_string, 4, NULL, 1);
// }

// void str_transfer(char *prompt)
// {
//     snprintf(prompt_string,PROMPT_STRING_MAX_LEN, "%s", prompt);
// }
