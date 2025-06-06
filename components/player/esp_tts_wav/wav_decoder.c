/* ------------------------------------------------------------------
 * Copyright (C) 2009 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include "wav_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TAG(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

struct wav_decoder {
	FILE *wav;
	uint32_t data_length;

	int format;
	int sample_rate;
	int bits_per_sample;
	int channels;
	int byte_rate;
	int block_align;
};

void print_wav_info(struct wav_decoder* wr)
{
	printf("WAV info:\n");
	printf( "format: %d\n", wr->format);
	printf( "sample rate: %d\n", wr->sample_rate);
	printf( "bits per sample: %d\n", wr->bits_per_sample);
	printf( "channels: %d\n", wr->channels);
	printf( "byte rate: %d\n", wr->byte_rate);
	printf( "block align: %d\n", wr->block_align);
	printf( "data length: %ld\n", wr->data_length);
}

static uint32_t read_tag(struct wav_decoder* wr) {
	uint32_t tag = 0;
	tag = (tag << 8) | fgetc(wr->wav);
	tag = (tag << 8) | fgetc(wr->wav);
	tag = (tag << 8) | fgetc(wr->wav);
	tag = (tag << 8) | fgetc(wr->wav);
	return tag;
}

static uint32_t read_int32(struct wav_decoder* wr) {
	uint32_t value = 0;
	value |= fgetc(wr->wav) <<  0;
	value |= fgetc(wr->wav) <<  8;
	value |= fgetc(wr->wav) << 16;
	value |= fgetc(wr->wav) << 24;
	return value;
}

static uint16_t read_int16(struct wav_decoder* wr) {
	uint16_t value = 0;
	value |= fgetc(wr->wav) << 0;
	value |= fgetc(wr->wav) << 8;
	return value;
}

void* wav_decoder_open(const char *filename) {
	struct wav_decoder* wr = (struct wav_decoder*) malloc(sizeof(*wr));
	long data_pos = 0;
	memset(wr, 0, sizeof(*wr));

	wr->wav = fopen(filename, "rb");
	if (wr->wav == NULL) {
		free(wr);
		return NULL;
	}

	while (1) {
		uint32_t tag, tag2, length;
		tag = read_tag(wr);
		if (feof(wr->wav))
			break;
		length = read_int32(wr);
		if (tag != TAG('R', 'I', 'F', 'F') || length < 4) {
			fseek(wr->wav, length, SEEK_CUR);
			continue;
		}
		tag2 = read_tag(wr);
		length -= 4;
		if (tag2 != TAG('W', 'A', 'V', 'E')) {
			fseek(wr->wav, length, SEEK_CUR);
			continue;
		}
		// RIFF chunk found, iterate through it
		while (length >= 8) {
			uint32_t subtag, sublength;
			subtag = read_tag(wr);
			if (feof(wr->wav))
				break;
			sublength = read_int32(wr);
			length -= 8;
			if (length < sublength)
				break;
			if (subtag == TAG('f', 'm', 't', ' ')) {
				if (sublength < 16) {
					// Insufficient data for 'fmt '
					break;
				}
				wr->format          = read_int16(wr);
				wr->channels        = read_int16(wr);
				wr->sample_rate     = read_int32(wr);
				wr->byte_rate       = read_int32(wr);
				wr->block_align     = read_int16(wr);
				wr->bits_per_sample = read_int16(wr);
			} else if (subtag == TAG('d', 'a', 't', 'a')) {
				data_pos = ftell(wr->wav);
				wr->data_length = sublength;
				fseek(wr->wav, sublength, SEEK_CUR);
			} else {
				fseek(wr->wav, sublength, SEEK_CUR);
			}
			length -= sublength;
		}
		if (length > 0) {
			// Bad chunk?
			fseek(wr->wav, length, SEEK_CUR);
		}
	}
	fseek(wr->wav, data_pos, SEEK_SET);
	// print_wav_info(wr);
	return wr;
}

void wav_decoder_close(void* obj) {
	struct wav_decoder* wr = (struct wav_decoder*) obj;
	fclose(wr->wav);
	free(wr);
}

int wav_decoder_get_header(void* obj, int* format, int* channels, int* sample_rate, int* bits_per_sample, unsigned int* data_length) {
	struct wav_decoder* wr = (struct wav_decoder*) obj;
	if (format)
		*format = wr->format;
	if (channels)
		*channels = wr->channels;
	if (sample_rate)
		*sample_rate = wr->sample_rate;
	if (bits_per_sample)
		*bits_per_sample = wr->bits_per_sample;
	if (data_length)
		*data_length = wr->data_length;
	return wr->format && wr->sample_rate;
}

int wav_decoder_run(void* obj, unsigned char* data, unsigned int length) {
	struct wav_decoder* wr = (struct wav_decoder*) obj;
	int n;
	if (wr->wav == NULL)
		return -1;
	if (length > wr->data_length)
		length = wr->data_length;
	n = fread(data, 1, length, wr->wav);
	wr->data_length -= length;
	return n;
}

int wav_decoder_get_channel(void* obj) {

	struct wav_decoder* wr = (struct wav_decoder*) obj;
	return wr->channels;
}

int wav_decoder_get_sample_rate(void* obj) {

	struct wav_decoder* wr = (struct wav_decoder*) obj;
	return wr->sample_rate;
}

int wav_decoder_get_data_length(void* obj) {

	struct wav_decoder* wr = (struct wav_decoder*) obj;
	return wr->data_length;
}