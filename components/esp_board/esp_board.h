#pragma once
#include "driver/gpio.h"
#include <stdbool.h>
#include "esp_err.h"

#define  USE_MIC 1
#define GPIO_MIC_I2S_LRCK      (GPIO_NUM_4)
#define GPIO_MIC_I2S_MCLK      (GPIO_NUM_NC)
#define GPIO_MIC_I2S_SCLK      (GPIO_NUM_5)
#define GPIO_MIC_I2S_DIN       (GPIO_NUM_6)
#define GPIO_I2S_DOUT           (GPIO_NUM_NC)

#define USE_SPEAKER 1
#define GPIO_SPEAKER_I2S_LRCK       (GPIO_NUM_16)
#define GPIO_SPEAKER_I2S_MCLK       (GPIO_NUM_NC)
#define GPIO_SPEAKER_I2S_SCLK       (GPIO_NUM_15)
#define GPIO_I2S_DIN                (GPIO_NUM_NC)
#define GPIO_SPEAKER_I2S_DOUT       (GPIO_NUM_7)

#define MIC_I2S_SIMPLEX_CONFIG_DEFAULT(sample_rate, channel_fmt, bits_per_chan) { \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate), \
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits_per_chan, channel_fmt), \
        .gpio_cfg = { \
            .mclk = GPIO_MIC_I2S_MCLK, \
            .bclk = GPIO_MIC_I2S_SCLK, \
            .ws   = GPIO_MIC_I2S_LRCK, \
            .dout = GPIO_I2S_DOUT, \
            .din  = GPIO_MIC_I2S_DIN, \
            .invert_flags = { \
                .mclk_inv = false, \
                .bclk_inv = false, \
                .ws_inv   = false, \
            }, \
        }, \
    }

#define SPEAKER_I2S_SIMPLEX_CONFIG_DEFAULT(sample_rate, channel_fmt, bits_per_chan) { \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate), \
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits_per_chan, channel_fmt), \
        .gpio_cfg = { \
        .mclk = GPIO_SPEAKER_I2S_MCLK, \
        .bclk = GPIO_SPEAKER_I2S_SCLK, \
        .ws   = GPIO_SPEAKER_I2S_LRCK, \
        .dout = GPIO_SPEAKER_I2S_DOUT, \
        .din  = GPIO_I2S_DIN, \
        .invert_flags = { \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv   = false, \
            }, \
        }, \
    }


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the board
 * 
 * @param sample_rate The sample rate of the board
 * @param channel_format The channel format of the board
 * @param bits_per_chan The bits per channel of the board
 * @return
**/
esp_err_t esp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan);

/**
 * @brief Get the record pcm data.
 * @param buffer The buffer where the data is stored.
 * @param buffer_len The buffer length.
 * @return
 *    - ESP_OK                  Success
 *    - Others                  Fail
 */
esp_err_t mic_read_data( int16_t *buffer, int buffer_len);

/**
 * @brief Write the pcm data to the speaker.
 * 
 * @param buffer The buffer where the data is stored.
 * @param buffer_len The buffer length.
 * @param output_volume The output volume.
 * @return
 *    - The number of bytes written to the speaker.
 */
int speaker_write_data(const int16_t *buffer, int buffer_len,  uint8_t output_volume);

/**
 * @brief Get the record channel number.
 * 
 * @return The record channel number.
 */
int esp_get_feed_channel(void);

/**
 * @brief esp_audio_play
 * 
 * @return - The number of bytes written to the speaker.
 */
int esp_audio_play(const int16_t* pcm_data, int length,uint8_t play_volume);

/**
 * @brief Get the input format of the board.
 * 
 * @return The input format of the board, like "MMR"
 */
char* esp_get_input_format(void);

#ifdef __cplusplus
}
#endif
