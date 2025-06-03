#include <stdio.h>
#include "esp_sr.h"
#include "esp_board.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 32));
    esp_sr_init();       //语音识别初始化
}

