#include "string.h"
#include "esp_board.h"
#include "driver/i2s_std.h"
#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define ADC_I2S_CHANNEL 1
static const char *TAG = "board";
static i2s_chan_handle_t rx_handle = NULL;        // I2S rx channel handler

static esp_err_t esp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, 32);
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    // std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;   //The default is I2S_MCLK_MULTIPLE_256. If not using 24-bit data width, 256 should be enough
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
    return ret_val;
}

static esp_err_t esp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;
    ret_val |= i2s_channel_disable(rx_handle);
    ret_val |= i2s_del_channel(rx_handle);
    rx_handle = NULL;
    return ret_val;
}

esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int audio_samples = buffer_len / (sizeof(int32_t));
    int32_t *temp_buffer = malloc(audio_samples * sizeof(int32_t));
    ret = i2s_channel_read(rx_handle, temp_buffer, audio_samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    // buffer = tmp_buff;
    // for (int i = 0; i < audio_samples; i++) {
    //     tmp_buff[i] = (tmp_buff[i] >> 14); // 32:8为有效位， 8:0为低8位， 全为0， AFE的输入为16位语音数据，拿29：13位是为了对语音信号放大。
    // }
    int samples = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples; i++) {
        int32_t value = temp_buffer[i] >> 12;
        buffer[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
    }
    free(temp_buffer);
    return ret;
}

int esp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}
char* esp_get_input_format(void)
{
    return "M";
}
esp_err_t esp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_i2s_init(I2S_NUM_AUTO, 16000, 1, 32);
    return ESP_OK;
}
