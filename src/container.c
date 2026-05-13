#include "container.h"

#include <string.h>
#include <stdio.h>

/*
	TODO: IMPLEMENT THE CONTAINERS
	
	A container will simply be a sequence of frames. Initially we'll have two
	containers -- a file container and a stream container.
	
	The file container will consist of a header, a sequence of frames and a
	checksum at the end. The first frames will consist of palettes and coding
	tables. They won't be repeated. The decoder will assume that the file is not
	corrupted.
	
	The stream container will not contain any predefined frame sequence and
	instead will just decode frames as they come in.
*/

#define HEADER_VERSION 2

#define pack_code(X,Y,Z,W) ((X) | ((Y)<<8) | ((Z)<<16) | ((W)<<24))

struct channel_data {
	int channel_flags;
	int channel_index;
	
	union {
		struct {
			
			// - copy of last frame
			// 
			
		} image;
		struct {
			
		} audio;
	};
};

struct file_header {
	int version;
	int flags;
};

struct channel_header {
	int channel_flags;
	int channel_index;
};

struct image_wavelet_header {
	int timestamp;
	int channel;
	
	short width;
	short height;
	int size;
};

struct delta_wavelet_header {
	
};

struct image_palette_header {
	
};

struct delta_palette_header {
	
};


void donk_create_encoder(donk_encoder_t* encoder, donk_stream_output_t* output, int mode) {
	memset(encoder, 0, sizeof(encoder));
	encoder->stream = output;
	donk_array_init(&encoder->channels, sizeof(struct channel_data), 0);
	
	struct file_header header;
	memset(&header, 0, sizeof(header));
	header.version = HEADER_VERSION;
	header.flags = 0;
	donk_write_bytes(encoder->stream, 8, "DONKFILE");
	donk_write_bytes(encoder->stream, sizeof(header), &header);
}

void donk_add_channel(donk_encoder_t* encoder, int flags, int index) {
	struct channel_data channel;
	memset(&channel, 0, sizeof(struct channel_data));
	channel.channel_flags = flags;
	channel.channel_index = index;
	donk_array_append(&encoder->channels, &channel);
	

	struct channel_header header;
	memset(&header, 0, sizeof(header));
	header.channel_flags = flags;
	header.channel_index = index;
	donk_write_bytes(encoder->stream, 8, "DONKCHAN");
	donk_write_bytes(encoder->stream, sizeof(header), &header);
}

void donk_encode_frame(donk_encoder_t* encoder, donk_frame_t* frame) {
	/*
	
	basic idea for the encoding process:
	1. encode as a new frame
	2. encode as a delta frame
	3. compare sizes
	4. best one add to output stream
	
	encoding a new frame:
	walk through all coding blocks
	for each block:
		- extract luma
		- extract chroma
		- downsample
		- ??
		
	TODO:
		- save image no compression
		- load image no compression
		- save/load with wavelets
		- drop some wavelets in zig zag
		- quantize wavelets
	
	*/
	
	// TODO: lookup channel info here

	struct image_wavelet_header header;
	memset(&header, 0, sizeof(header));
	header.timestamp = frame->timestamp;
	header.channel = frame->channel;
	header.width = frame->image.width;
	header.height = frame->image.height;
	header.size = frame->image.width * frame->image.height * 3;
	donk_write_bytes(encoder->stream, 8, "DONKIMWV");
	donk_write_bytes(encoder->stream, sizeof(header), &header);
	donk_write_bytes(encoder->stream, header.size, frame->image.pixels);	
}



void donk_create_decoder(donk_decoder_t* decoder, donk_stream_input_t* input) {
	memset(decoder, 0, sizeof(decoder));
	decoder->stream = input;
	donk_array_init(&decoder->channels, sizeof(struct channel_data), 0);
}

static void decode_file_info(donk_decoder_t* decoder, donk_frame_t* frame) {
	struct file_header header;
	donk_read_bytes(decoder->stream, sizeof(header), &header);
	
	printf("loading file version %i flags %i\n", header.version, header.flags);
	if (header.version != HEADER_VERSION) {
		printf("warning: file is version %i not %i\n", header.version, HEADER_VERSION);
	}
}

static void decode_channel_info(donk_decoder_t* decoder, donk_frame_t* frame) {
	struct channel_header header;
	donk_read_bytes(decoder->stream, sizeof(header), &header);
	
	printf("found channel %i flags %i\n", header.channel_index, header.channel_flags);
	
	struct channel_data channel;
	memset(&channel, 0, sizeof(struct channel_data));
	channel.channel_flags = header.channel_flags;
	channel.channel_index = header.channel_index;
	donk_array_append(&decoder->channels, &channel);
}

static void decode_wavelet_image(donk_decoder_t* decoder, donk_frame_t* frame) {
	struct image_wavelet_header header;
	donk_read_bytes(decoder->stream, sizeof(header), &header);
	
	printf("decoding image channel %i timestamp %i\n", header.channel, header.timestamp);
	printf("image width %i height %i\n", header.width, header.height);
	
	donk_frame_image_make(frame, header.width, header.height); 
	frame->timestamp = header.timestamp;
	frame->channel = header.channel;
	frame->image.width = header.width;
	frame->image.height = header.height;
	
	donk_read_bytes(decoder->stream, header.size, frame->image.pixels);
	
	printf("done decoding image\n");
}

void donk_decode_frame(donk_decoder_t* decoder, donk_frame_t* frame) {
	int sync;
	donk_read_bytes(decoder->stream, 4, &sync);
	if (sync != pack_code('D', 'O', 'N', 'K')) {
		// TODO: seek to next sync word
	}
	
	int code;
	donk_read_bytes(decoder->stream, 4, &code);
	
	switch (code) {
		case pack_code('F', 'I', 'L', 'E'):
			decode_file_info(decoder, frame);
			donk_decode_frame(decoder, frame);
			return;
		case pack_code('C', 'H', 'A', 'N'):
			decode_file_info(decoder, frame);
			donk_decode_frame(decoder, frame);
			return;
		
		case pack_code('I', 'M', 'W', 'V'):
			decode_wavelet_image(decoder, frame); return;
		default:
			printf("frame code '%c%c%c%c' invalid\n", code & 0xFF,
				(code >> 8) & 0xFF, (code >> 16) & 0xFF, (code >> 24) & 0xFF);
				{static int errs = 0; if (errs++ > 10) abort();}
			return;
	}
	
}



