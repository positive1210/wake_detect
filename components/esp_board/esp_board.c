#include "string.h"
#include "math.h"
#include "esp_board.h"
#include "driver/i2s_std.h"
#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_I2S_CHANNEL 1
static i2s_chan_handle_t rx_handle = NULL;        // I2S rx channel handler
static i2s_chan_handle_t tx_handle = NULL;        // I2S tx channel handler

static esp_err_t mic_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg =MIC_I2S_SIMPLEX_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, 32);
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    // std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;   //The default is I2S_MCLK_MULTIPLE_256. If not using 24-bit data width, 256 should be enough
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
    return ret_val;
}


static esp_err_t speaker_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    i2s_std_config_t std_cfg = SPEAKER_I2S_SIMPLEX_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, 32);
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ret_val |= i2s_channel_init_std_mode(tx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(tx_handle);
    return ret_val;
}


static esp_err_t esp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;
    ret_val |= i2s_channel_disable(rx_handle);
    ret_val |= i2s_del_channel(rx_handle);
    ret_val |= i2s_channel_disable(tx_handle);
    ret_val |= i2s_del_channel(tx_handle);
    rx_handle = NULL;
    tx_handle = NULL;
    return ret_val;
}


esp_err_t mic_read_data(int16_t *buffer, int audio_samples)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int32_t *temp_buffer = malloc(audio_samples * sizeof(int32_t));
    ret = i2s_channel_read(rx_handle, temp_buffer, audio_samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    // printf("transform before audio data:\t%ld,\t%d,\t%d,\t%d\n",temp_buffer[0],temp_buffer[1],temp_buffer[2],temp_buffer[3]);
    // vTaskDelay(100/portTICK_PERIOD_MS);
    int samples = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples; i++) {
        int32_t value = (temp_buffer[i] >>12)*2;
        buffer[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
    }
    free(temp_buffer);
    return ret;
}


int speaker_write_data(const int16_t *buffer, int audio_samples,  uint8_t output_volume)
{
    size_t bytes_write;
    int32_t *temp_buffer = malloc(audio_samples * sizeof(int32_t));
    int32_t volume_factor = pow((double)output_volume/ 100.0, 2) * 65536;
        for (int i = 0; i < audio_samples; i++) {
        int64_t temp = (int64_t)buffer[i]* volume_factor; // 使用 int64_t 进行乘法运算

        if (temp > INT32_MAX) {
            temp_buffer[i] = INT32_MAX;
        } else if (temp < INT32_MIN) {
            temp_buffer[i] = INT32_MIN;
        } else {
            temp_buffer[i] = (int32_t)temp;
        }
    }
    ESP_ERROR_CHECK(i2s_channel_write(tx_handle, temp_buffer, audio_samples * sizeof(int32_t), &bytes_write, portMAX_DELAY));
    free(temp_buffer);
    return bytes_write;
}

int esp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

int esp_audio_play(const int16_t* pcm_data, int length,uint8_t play_volume)
{
    return speaker_write_data(pcm_data,length/sizeof(int16_t),play_volume);
}

char* esp_get_input_format(void)
{
    return "M";
}

esp_err_t esp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    mic_i2s_init(I2S_NUM_0, 16000, 1, 32);
    speaker_i2s_init(I2S_NUM_1, 16000, 1, 32);
    return ESP_OK;
}





