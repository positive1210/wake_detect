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
static int s_play_sample_rate = 16000;
static int s_play_channel_format = 1;
static int s_bits_per_chan = 16;
static i2s_chan_handle_t                rx_handle = NULL;        // I2S rx channel handler

static esp_err_t esp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t rx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        // .clk_cfg = {
        //     .sample_rate_hz = 16000,
        //     .clk_src = I2S_CLK_SRC_DEFAULT,
        //     .mclk_multiple = I2S_MCLK_MULTIPLE_384, // 确保mclk_multiple是3的倍数
        // },
        // .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT, .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT, .slot_mode = I2S_SLOT_MODE_MONO, .slot_mask = I2S_STD_SLOT_LEFT, .ws_width = I2S_SLOT_BIT_WIDTH_32BIT, .ws_pol = false, .bit_shift = true, .left_align = true, .big_endian = false, .bit_order_lsb = false},
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, // some codecs may require mclk signal, this example doesn't need it
            .bclk = GPIO_I2S_SCLK, 
            .ws   = GPIO_I2S_LRCK, 
            .dout = I2S_GPIO_UNUSED, 
            .din  = GPIO_I2S_DIN, 
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    // i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, 32);
    // std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    // std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;   //The default is I2S_MCLK_MULTIPLE_256. If not using 24-bit data width, 256 should be enough
    ret_val |= i2s_channel_init_std_mode(rx_handle, &rx_std_cfg);
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

// esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
// {
//     esp_err_t ret = ESP_OK;
//     size_t bytes_read;
//     int audio_chunksize = buffer_len / (sizeof(int32_t));
//     ret = i2s_channel_read(rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);
//     int32_t *tmp_buff = buffer;
//     for (int i = 0; i < audio_chunksize; i++) {
//         tmp_buff[i] = (tmp_buff[i] >> 14); // 32:8为有效位， 8:0为低8位， 全为0， AFE的输入为16位语音数据，拿29：13位是为了对语音信号放大。
//     }
//     return ret;
// }
esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int audio_chunksize = buffer_len / (sizeof(int32_t));
    ret = i2s_channel_read(rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);
    
    int32_t *tmp_buff = buffer;
    // for (int i = 0; i < audio_chunksize; i++) {
    //     tmp_buff[i] = (tmp_buff[i] >> 14); // 32:8为有效位， 8:0为低8位， 全为0， AFE的输入为16位语音数据，拿29：13位是为了对语音信号放大。
    // }
    for (int i = 0; i <= bytes_read - 4; i += 4)
            {
                uint8_t byte4 = tmp_buff[i];
                uint8_t byte3 = tmp_buff[i + 1];
                uint8_t byte2 = tmp_buff[i + 2];
                uint8_t byte1 = tmp_buff[i + 3];

                // uint32_t sample = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
                //     // printf("Bytes: [0x%02x, 0x%02x, 0x%02x, 0x%02x] -> Sample: 0x%08lx\n", byte1, byte2, byte3, byte4, sample);
                int32_t sample = (byte1 << 16) | (byte2 << 8) | (byte3 << 0);

                if (sample & 0x00800000)
                    sample |= 0xFF000000;
                printf("%ld\n", sample * 100);
           

                // printf("%08ld\n", w_buf[i / 4]);
            }
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
    esp_i2s_init(I2S_NUM_AUTO, 16000, 2, 32);
    return ESP_OK;
}
