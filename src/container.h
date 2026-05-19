#ifndef DONK_CONTAINER_H
#define DONK_CONTAINER_H

#include "array.h"
#include "stream.h"
#include "frame.h"

typedef struct {
	donk_array_t channels;
	donk_stream_output_t* stream;
	int mode;
} donk_encoder_t;

typedef struct {
	donk_array_t channels;
	donk_stream_input_t* stream;
} donk_decoder_t;

enum {
	DONK_ENCODER_FILE = 1,
	DONK_ENCODER_STREAM = 2,
};

enum {
	DONK_CHANNEL_IMAGE = 1,
	DONK_CHANNEL_AUDIO = 2,
	DONK_CHANNEL_USE_PALETTE = 4,
};

enum {
	DONK_DEFAULT_IMAGE = 1,
	DONK_DEFAULT_VIDEO = 2,
	DONK_DEFAULT_AUDIO_L = 3,
	DONK_DEFAULT_AUDIO_R = 4,
};

void donk_create_encoder(donk_encoder_t* encoder, donk_stream_output_t* output, int mode);
void donk_add_channel(donk_encoder_t* encoder, int flags, int index);
void donk_encode_frame(donk_encoder_t* encoder, donk_frame_t* frame);
void donk_add_palette(donk_encoder_t* encoder, donk_palette_t* palette, int channel_index);

void donk_create_decoder(donk_decoder_t* decoder, donk_stream_input_t* input);
void donk_decode_frame(donk_decoder_t* decoder, donk_frame_t* frame);

#endif // DONK_CONTAINER_H