#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "string.h"
#include "esp_sr.h"
#include "drugs_action.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "esp_board.h"
#include "model_path.h"


static const char *TAG = "esp_sr";
static srmodel_list_t *models = NULL;
static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data= NULL;
static volatile int task_flag = 0;
int detect_flag = 0;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);//获取音频块大小
    int nch = afe_handle->get_feed_channel_num(afe_data);//获取声道数
    int feed_channel = esp_get_feed_channel();//获取ADC输入通道
    assert(nch==feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);
    while (task_flag) {
        mic_read_data(i2s_buff, audio_chunksize * feed_channel);
        // printf("transformed audio data:\t%d,\t%d,\t%d,\t%d\n",i2s_buff[0],i2s_buff[1],i2s_buff[2],i2s_buff[3]);
        // vTaskDelay(100/portTICK_PERIOD_MS);
        afe_handle->feed(afe_data, i2s_buff);//将从I2S接口获取的音频数据传递给afe_data
    }

    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;  // 接收参数
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);  // 获取fetch帧长度
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE); // 初始化命令词模型
    printf("multinet:%s\n", mn_name); // 打印命令词模型名称
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 10000);  // 设置唤醒后等待事件 6000代表6000毫秒，6秒后退出命令词识别
    esp_mn_commands_clear(); // 清除当前的命令词列表
    esp_mn_commands_add(1, "jia yao"); // 加药
    esp_mn_commands_add(2, "que ren jia yao"); // 确认加药
    esp_mn_commands_update(); // 更新命令词
    int mn_chunksize = multinet->get_samp_chunksize(model_data);  // 获取samp帧长度
    assert(mn_chunksize == afe_chunksize);

    // 打印所有的命令词
    multinet->print_active_speech_commands(model_data);
    printf("------------detect start------------\n");
    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); // 获取模型输出结果
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "fetch error!\n");
            break;
        }
        // if (res->wakeup_state == WAKENET_DETECTED) {
        //     ESP_LOGI(TAG, "WAKEWORD DETECTED\n");
	    //     multinet->clean(model_data);  // clean all status of multinet
        // } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {  // 检测到唤醒词
        //     // play_voice = -1;
        //     afe_handle->disable_wakenet(afe_data);  // 关闭唤醒词识别
        //     detect_flag = 1; // 标记已检测到唤醒词
        //     ESP_LOGI(TAG, "AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
        // }
        if (res->wakeup_state == WAKENET_DETECTED) {
            multinet->clean(model_data);  // clean all status of multinet
            afe_handle->disable_wakenet(afe_data);  // 关闭唤醒词识别
            detect_flag = 1; // 标记已检测到唤醒词
            ESP_LOGI(TAG, "WAKEWORD DETECTED\n");
        }
        if (detect_flag == 1) {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data); // 检测命令词
            if (mn_state == ESP_MN_STATE_DETECTING) {
                continue;
            }
            if (mn_state == ESP_MN_STATE_DETECTED) { // 已检测到命令词
                esp_mn_results_t *mn_result = multinet->get_results(model_data); // 获取检测词结果
                for (int i = 0; i < mn_result->num; i++) { // 打印获取到的命令词
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, string:%s prob: %f\n", 
                    i+1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                }
                // 根据命令词 执行相应动作
                switch (mn_result->command_id[0])
                {
                    case 1: // jia yao 加药
                        add_drugs();
                        break;
                    case 2: // que ren jia yao 确认加药
                        confirm_add_drugs();
                        break;
                    default:
                        break;
                }
                ESP_LOGI(TAG, "\n-----------listening-----------\n");
            }
            if (mn_state == ESP_MN_STATE_TIMEOUT) { // 达到最大检测命令词时间
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                ESP_LOGI(TAG, "timeout, string:%s\n", mn_result->string);
                afe_handle->enable_wakenet(afe_data);  // 重新打开唤醒词识别
                detect_flag = 0; // 清除标记
                ESP_LOGI(TAG, "\n-----------awaits to be waken up-----------\n");
                continue;
            }
        }
    }

    if (model_data) {
        multinet->destroy(model_data);
        model_data = NULL;
    }
    vTaskDelete(NULL);
}

void esp_sr_init(void)
{
    models = esp_srmodel_init("model");
    // 打印所有模型
    if (models) {
        for (int i=0; i<models->num; i++) {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
                ESP_LOGI(TAG, "wakenet model in flash: %s\n", models->model_name[i]);
            }
        }
    }

    // afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    // print/modify wake word model. 
    if (afe_config->wakenet_model_name) {
        ESP_LOGI(TAG, "wakeword model in AFE config: %s\n", afe_config->wakenet_model_name);
    }
    if (afe_config->wakenet_model_name_2) {
        ESP_LOGI(TAG, "wakeword model in AFE config: %s\n", afe_config->wakenet_model_name_2);
    }

    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_data = afe_handle->create_from_config(afe_config);

    // afe_config_free(afe_config);
    // 初始化任务
    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8*1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4*1024, (void*)afe_data, 5, NULL, 1);
}
