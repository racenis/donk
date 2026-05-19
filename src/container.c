#include "container.h"

#include "coding.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
			
			donk_palette_t* palette;
			
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

struct palette_header {
	int channel;
	unsigned short colors[256];
	unsigned char subpalettes[512];
};

struct image_wavelet_header {
	int timestamp;
	int channel;
	
	short width;
	short height;
	int size;
	int actual_size;
	int flags;
};

struct delta_wavelet_header {

};

struct image_palette_header {
	int timestamp;
	int channel;
	
	short width;
	short height;
	int size;
	int actual_size;
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

static void interlace_coding_block(donk_frame_t* frame, int w, int h,
						unsigned char* r, unsigned char* g, unsigned char* b)
{
	int w_extent = w + 16 > frame->image.width ? frame->image.width : w + 16;
	int h_extent = h + 16 > frame->image.height ? frame->image.height : h + 16;
	
	for (int y = h; y < h_extent; y++)
	for (int x = w; x < w_extent; x++) {
		int destination_offset = 3 * (y * frame->image.width + x);
		int source_offset = (y - h) * 16 + x - w;
		frame->image.pixels[destination_offset + 0] = r[source_offset];
		frame->image.pixels[destination_offset + 1] = g[source_offset];
		frame->image.pixels[destination_offset + 2] = b[source_offset];
	}
}

static void deinterlace_coding_block(donk_frame_t* frame, int w, int h,
						unsigned char* r, unsigned char* g, unsigned char* b)
{
	int w_extent = w + 16 > frame->image.width ? frame->image.width : w + 16;
	int h_extent = h + 16 > frame->image.height ? frame->image.height : h + 16;
	
	for (int y = h; y < h_extent; y++)
	for (int x = w; x < w_extent; x++) {
		int source_offset = 3 * (y * frame->image.width + x);
		int destination_offset = (y - h) * 16 + x - w;
		r[destination_offset] = frame->image.pixels[source_offset + 0];
		g[destination_offset] = frame->image.pixels[source_offset + 1];
		b[destination_offset] = frame->image.pixels[source_offset + 2];
	}
}

static void encode_wavelet_block(donk_stream_output_t* stream,
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	int luma[256];
	int cr[256];
	int cb[256];
	
	for (int i = 0; i < 256; i++) {
		luma[i] = donk_luma_from_rgb((int)r[i], (int)g[i], (int)b[i]);
		cr[i] = donk_chroma_from_ry((int)r[i], luma[i]);
		cb[i] = donk_chroma_from_by((int)b[i], luma[i]);
	}
	
	donk_wavelet_transform(luma, 16);
	donk_wavelet_transform(cr, 16);
	donk_wavelet_transform(cb, 16);
	
	int luma_cutoff_a = donk_wavelet_cutoff(luma, 256, donk_wavelet_pattern_a, 10, 32);
	int luma_cutoff_b = donk_wavelet_cutoff(luma, 256, donk_wavelet_pattern_b, 10, 32);
	int luma_cutoff_c = donk_wavelet_cutoff(luma, 256, donk_wavelet_pattern_c, 10, 32);
	int luma_cutoff_d = donk_wavelet_cutoff(luma, 256, donk_wavelet_pattern_d, 10, 32);
	
	unsigned char wavelet_pattern = 0;
	int luma_cutoff = luma_cutoff_a;
	if (luma_cutoff_b < luma_cutoff) luma_cutoff = luma_cutoff_b, wavelet_pattern = 1;
	if (luma_cutoff_c < luma_cutoff) luma_cutoff = luma_cutoff_c, wavelet_pattern = 2;
	if (luma_cutoff_d < luma_cutoff) luma_cutoff = luma_cutoff_d, wavelet_pattern = 3;
	
	int chroma_cutoff_cr = donk_wavelet_cutoff(cr, 256, donk_wavelet_pattern_d, 3, 32);
	int chroma_cutoff_cb = donk_wavelet_cutoff(cb, 256, donk_wavelet_pattern_d, 3, 32);
	int chroma_cutoff = chroma_cutoff_cr > chroma_cutoff_cb ? chroma_cutoff_cr : chroma_cutoff_cb;
	if (chroma_cutoff >= 15) chroma_cutoff = 15;
	
	for (int i = 0; i < 256; i++) {
		if (cr[i] < -128) cr[i] = -128;
		if (cr[i] > 127) cr[i] = 127;
		if (cb[i] < -128) cb[i] = -128;
		if (cb[i] > 127) cb[i] = 127;
		
		if (i == 0) continue;
		
		if (luma[i] < -128) luma[i] = -128;
		if (luma[i] > 127) luma[i] = 127;
	}

	unsigned char packed_luma[256];
	unsigned char packed_cr[256];
	unsigned char packed_cb[256];
	
	const int* luma_pattern;
	switch (wavelet_pattern) {
		case 0: luma_pattern = donk_wavelet_pattern_a;
		case 1: luma_pattern = donk_wavelet_pattern_b;
		case 2: luma_pattern = donk_wavelet_pattern_c;
		case 3: luma_pattern = donk_wavelet_pattern_d;
	}

	for (int i = 0; i < 256; i++) {
		packed_luma[i] = luma[luma_pattern[i]];
		packed_cr[i] = cr[donk_wavelet_pattern_d[i]];
		packed_cb[i] = cb[donk_wavelet_pattern_d[i]];
	}
	
	unsigned char quantized_luma[256];
	unsigned char quantized_cr[256];
	unsigned char quantized_cb[256];
	
	int cr_bits = donk_quantize_bits(packed_cr, 256, 7);
	int cb_bits = donk_quantize_bits(packed_cb, 256, 7);
	
	unsigned char luma_bits = donk_quantize_bits(packed_luma, 256, 8);
	unsigned char chroma_bits = cr_bits > cb_bits ? cr_bits : cb_bits;
	
	donk_quantize(packed_luma, luma_cutoff, luma_bits, quantized_luma);
	donk_quantize(packed_cr, chroma_cutoff, chroma_bits, quantized_cr);
	donk_quantize(packed_cb, chroma_cutoff, chroma_bits, quantized_cb);
	int luma_bytes = donk_quantized_bytes(luma_cutoff, luma_bits);
	int chroma_bytes = donk_quantized_bytes(chroma_cutoff, chroma_bits);
	
	unsigned char w_cutoff = luma_cutoff;
	unsigned char chroma_pack = wavelet_pattern | (((unsigned char)chroma_cutoff) << 2);
	unsigned char quantization = luma_bits << 4 | chroma_bits;
	
	donk_write_bytes(stream, 1, &w_cutoff);
	donk_write_bytes(stream, 1, &chroma_pack);
	donk_write_bytes(stream, 1, &quantization);
	donk_write_bytes(stream, luma_bytes, quantized_luma);
	
	donk_write_bytes(stream, chroma_bytes, quantized_cr);
	donk_write_bytes(stream, chroma_bytes, quantized_cb);
}

static void encode_palette_block(donk_stream_output_t* stream,
	donk_palette_t* palette, unsigned char* r, unsigned char* g, unsigned char* b)
{
	unsigned char subpalette = donk_palette_best_subpalette(palette, r, g, b);
	
	unsigned char palettized_result[256];
	donk_palette_subpalettize(palette, subpalette, r, g, b, palettized_result);
	
	unsigned char packed_result[128];
	for (int i = 0; i < 128; i++) {
		packed_result[i] = (palettized_result[i * 2] << 4) | palettized_result[i * 2 + 1];
	}
	
	donk_write_bytes(stream, 1, &subpalette);
	donk_write_bytes(stream, 128, packed_result);
}

void donk_encode_frame(donk_encoder_t* encoder, donk_frame_t* frame) {
	struct channel_data* channel = NULL;
	donk_array_for_each(&encoder->channels, candidate, struct channel_data) {
		if (candidate->channel_index == frame->channel) {
			channel = candidate;
			break;
		}
	}
	
	if (!channel) abort();
	
	if (channel->channel_flags & (DONK_CHANNEL_IMAGE | DONK_CHANNEL_USE_PALETTE)) {
		int horizontal_blocks = (frame->image.width + 15) & ~15;
		int vertical_blocks = (frame->image.height + 15) & ~15;
		int palette_data_size = horizontal_blocks * vertical_blocks * (1 + 256);
		
		unsigned char* encoded_palette_data = malloc(palette_data_size);
		
		donk_stream_output_t encode_stream;
		donk_open_output_stream_in_memory(&encode_stream, encoded_palette_data, palette_data_size);
		
		for (int h = 0; h < frame->image.height; h += 16)
		for (int w = 0; w < frame->image.width; w += 16) {
			unsigned char r[256];
			unsigned char g[256];
			unsigned char b[256];
			
			deinterlace_coding_block(frame, w, h, r, g, b);
			
			encode_palette_block(&encode_stream, channel->image.palette, r, g, b);
		}
		
		// * here we would encode a delta frame *
		
		struct image_palette_header header;
		memset(&header, 0, sizeof(header));
		header.timestamp = frame->timestamp;
		header.channel = frame->channel;
		header.width = frame->image.width;
		header.height = frame->image.height;
		header.size = encode_stream.cursor;
		header.actual_size = encode_stream.cursor;
		donk_write_bytes(encoder->stream, 8, "DONKIMPT");
		donk_write_bytes(encoder->stream, sizeof(header), &header);
		
		donk_write_bytes(encoder->stream, encode_stream.cursor, encoded_palette_data);
		
		return;
	}
	
	if (channel->channel_flags & DONK_CHANNEL_IMAGE) {
		int horizontal_blocks = (frame->image.width + 15) & ~15;
		int vertical_blocks = (frame->image.height + 15) & ~15;
		int wavelet_data_max = horizontal_blocks * vertical_blocks * (3 + 3 * 256);
		
		unsigned char* encoded_wavelet_data = malloc(wavelet_data_max);
		
		donk_stream_output_t encode_stream;
		donk_open_output_stream_in_memory(&encode_stream, encoded_wavelet_data, wavelet_data_max);
		
		for (int h = 0; h < frame->image.height; h += 16)
		for (int w = 0; w < frame->image.width; w += 16) {
			unsigned char r[256];
			unsigned char g[256];
			unsigned char b[256];
			
			deinterlace_coding_block(frame, w, h, r, g, b);
			
			encode_wavelet_block(&encode_stream, r, g, b);
		}
		
		// * here we would encode a delta frame *
		
		struct image_wavelet_header header;
		memset(&header, 0, sizeof(header));
		header.timestamp = frame->timestamp;
		header.channel = frame->channel;
		header.width = frame->image.width;
		header.height = frame->image.height;
		header.size = encode_stream.cursor;
		header.actual_size = encode_stream.cursor;
		donk_write_bytes(encoder->stream, 8, "DONKIMWV");
		donk_write_bytes(encoder->stream, sizeof(header), &header);
		
		donk_write_bytes(encoder->stream, encode_stream.cursor, encoded_wavelet_data);
		
		return;
	}
	
	printf("o no couldent encode the frame!!!!\n");
}

void donk_add_palette(donk_encoder_t* encoder, donk_palette_t* palette, int channel_index) {
	struct channel_data* channel = NULL;
	donk_array_for_each(&encoder->channels, candidate, struct channel_data) {
		if (candidate->channel_index == channel_index) {
			channel = candidate;
			break;
		}
	}
	
	if (!channel) abort();
	
	channel->image.palette = palette;
	
	struct palette_header header;
	memset(&header, 0, sizeof(header));
	header.channel = channel_index;
	
	memcpy(header.colors, palette->colors, 512);
	memcpy(header.subpalettes, palette->subpalettes, 512);
	
	donk_write_bytes(encoder->stream, 8, "DONKPLTE");
	donk_write_bytes(encoder->stream, sizeof(header), &header);
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

static void decode_palette_info(donk_decoder_t* decoder, donk_frame_t* frame) {
	struct palette_header header;
	donk_read_bytes(decoder->stream, sizeof(header), &header);
	
	printf("found palete for %i \n", header.channel);

	donk_palette_t* palette = malloc(sizeof(donk_palette_t));
	donk_palette_make(palette);
	
	palette->colors = malloc(512);
	palette->subpalettes = malloc(512);
	
	memcpy(palette->colors, header.colors, 512);
	memcpy(palette->subpalettes, header.subpalettes, 512);
	
	struct channel_data* channel = NULL;
	donk_array_for_each(&decoder->channels, candidate, struct channel_data) {
		if (candidate->channel_index == header.channel) {
			channel = candidate;
			break;
		}
	}
	
	channel->image.palette = palette;
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
	
	int horizontal_blocks = (header.width >> 4) << 4 == header.width 
		? header.width : ((header.width >> 4) + 1) << 4;
	int vertical_blocks = (header.height >> 4) << 4 == header.height 
		? header.height : ((header.height >> 4) + 1) << 4;
	for (int h = 0; h < vertical_blocks; h += 16)
	for (int w = 0; w < horizontal_blocks; w += 16) {
		unsigned char quantized_luma[256];
		unsigned char quantized_cr[256];
		unsigned char quantized_cb[256];
		
		char packed_luma[256];
		char packed_cr[256];
		char packed_cb[256];
		
		for (int i = 0; i < 256; i++) packed_luma[i] = 0;
		for (int i = 0; i < 256; i++) packed_cr[i] = 0;
		for (int i = 0; i < 256; i++) packed_cb[i] = 0;
		
		unsigned char lumas;
		unsigned char chroma_pack;
		unsigned char quantization;
		donk_read_bytes(decoder->stream, 1, &lumas);
		donk_read_bytes(decoder->stream, 1, &chroma_pack);
		donk_read_bytes(decoder->stream, 1, &quantization);
		
		unsigned char chromas = chroma_pack >> 2;
		unsigned char wavelet_pattern = 0x03 & chroma_pack;
		const int* luma_pattern;
		switch (wavelet_pattern) {
			case 0: luma_pattern = donk_wavelet_pattern_a;
			case 1: luma_pattern = donk_wavelet_pattern_b;
			case 2: luma_pattern = donk_wavelet_pattern_c;
			case 3: luma_pattern = donk_wavelet_pattern_d;
		}
		
		unsigned char luma_bits = quantization >> 4;
		unsigned char chroma_bits = quantization & 0x0F;
		
		int luma_bytes = donk_quantized_bytes(lumas, luma_bits);
		int chroma_bytes = donk_quantized_bytes(chromas, chroma_bits);
		
		donk_read_bytes(decoder->stream, luma_bytes, quantized_luma);
		donk_read_bytes(decoder->stream, chroma_bytes, quantized_cr);
		donk_read_bytes(decoder->stream, chroma_bytes, quantized_cb);
		
		donk_dequantize(packed_luma, lumas, luma_bits, quantized_luma);
		donk_dequantize(packed_cr, chromas, chroma_bits, quantized_cr);
		donk_dequantize(packed_cb, chromas, chroma_bits, quantized_cb);
		
		int luma[256];
		int cr[256];
		int cb[256];
		
		memset(luma, 0, 1024);
		memset(cr, 0, 1024);
		memset(cb, 0, 1024);
		
		luma[0] = (unsigned char)packed_luma[0];
		cr[0] = packed_cr[0];
		cb[0] = packed_cb[0];
		
		for (int i = 1; i < 256; i++) {
			luma[luma_pattern[i]] = packed_luma[i];
			cr[donk_wavelet_pattern_d[i]] = packed_cr[i];
			cb[donk_wavelet_pattern_d[i]] = packed_cb[i];
		}
		
		donk_inv_wavelet_transform(luma, 16);
		donk_inv_wavelet_transform(cr, 16);
		donk_inv_wavelet_transform(cb, 16);
		
		unsigned char r_deint[256];
		unsigned char g_deint[256];
		unsigned char b_deint[256];
		
		for (int i = 0; i < 256; i++) {
			int luma_val = luma[i];
			int cr_val = cr[i];
			int cb_val = cb[i];
			
			int r = donk_r_from_ycrcb(cr_val, cb_val, luma_val);
			int b = donk_b_from_ycrcb(cr_val, cb_val, luma_val);

			if (r < 0) r = 0;
			if (r > 255) r = 255;
			if (b < 0) b = 0;
			if (b > 255) b = 255;
			
			int g = donk_g_from_yrb(r, b, luma_val);
			
			if (g < 0) g = 0;
			if (g > 255) g = 255;
			
			r_deint[i] = r;
			g_deint[i] = g;
			b_deint[i] = b;
		}
		
		interlace_coding_block(frame, w, h, r_deint, g_deint, b_deint);
	}
	
	printf("done decoding image\n");
}

static void decode_palette_image(donk_decoder_t* decoder, donk_frame_t* frame) {
	struct image_palette_header header;
	donk_read_bytes(decoder->stream, sizeof(header), &header);
	
	printf("decoding palette image channel %i timestamp %i\n", header.channel, header.timestamp);
	printf("image width %i height %i\n", header.width, header.height);
	
	struct channel_data* channel = NULL;
	donk_array_for_each(&decoder->channels, candidate, struct channel_data) {
		if (candidate->channel_index == header.channel) {
			channel = candidate;
			break;
		}
	}
	
	if (!channel) abort();
	
	donk_frame_image_make(frame, header.width, header.height); 
	frame->timestamp = header.timestamp;
	frame->channel = header.channel;
	frame->image.width = header.width;
	frame->image.height = header.height;
	
	
	int horizontal_blocks = (header.width >> 4) << 4 == header.width 
		? header.width : ((header.width >> 4) + 1) << 4;
	int vertical_blocks = (header.height >> 4) << 4 == header.height 
		? header.height : ((header.height >> 4) + 1) << 4;
	for (int h = 0; h < vertical_blocks; h += 16)
	for (int w = 0; w < horizontal_blocks; w += 16) {
		unsigned char palettized_block[256];
		unsigned char packed_block[128];
		unsigned char palette;
		
		donk_read_bytes(decoder->stream, 1, &palette);
		donk_read_bytes(decoder->stream, 128, packed_block);
		
		for (int i = 0; i < 128; i++) {
			palettized_block[i * 2] = packed_block[i] >> 4;
			palettized_block[i * 2 + 1] = packed_block[i] & 0x0F;
		}
		
		unsigned char r[256];
		unsigned char g[256];
		unsigned char b[256];
		
		donk_palette_desubpalettize(channel->image.palette, palette, r, g, b, palettized_block);
		
		interlace_coding_block(frame, w, h, r, g, b);
	}
	
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
			decode_channel_info(decoder, frame);
			donk_decode_frame(decoder, frame);
			return;
		case pack_code('P', 'L', 'T', 'E'):
			decode_palette_info(decoder, frame);
			donk_decode_frame(decoder, frame);
			return;
		
		case pack_code('I', 'M', 'W', 'V'):
			decode_wavelet_image(decoder, frame); return;
		case pack_code('I', 'M', 'P', 'T'):
			decode_palette_image(decoder, frame); return;
		default:
			printf("frame code '%c%c%c%c' invalid\n", code & 0xFF,
				(code >> 8) & 0xFF, (code >> 16) & 0xFF, (code >> 24) & 0xFF);
				{static int errs = 0; if (errs++ > 10) abort();}
			return;
	}
	
}



