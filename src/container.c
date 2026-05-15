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
	int actual_size;
	int flags;
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

	for (int h = 0; h < header.height; h += 16)
	for (int w = 0; w < header.width; w += 16) {
		int w_extent = w + 16 > header.width ? header.width : w + 16;
		int h_extent = h + 16 > header.height ? header.height : h + 16;
		
		int tluma[16 * 16];
		int tcr[16 * 16];
		int tcb[16 * 16];
		
		for (int y = h; y < h_extent; y++)
		for (int x = w; x < w_extent; x++) {
			int source_offset = 3 * (y * header.width + x);
			int r = frame->image.pixels[source_offset + 0];
			int g = frame->image.pixels[source_offset + 1];
			int b = frame->image.pixels[source_offset + 2];
			
			int luma = donk_luma_from_rgb(r, g, b);
			int cr = donk_chroma_from_ry(r, luma);
			int cb = donk_chroma_from_by(b, luma);
			
			int destination_offset = (y - h) * 16 + x - w;
			tluma[destination_offset] = luma;
			tcr[destination_offset] = cr;
			tcb[destination_offset] = cb;
		}
		
		donk_wavelet_transform(tluma, 16);
		donk_wavelet_transform(tcr, 16);
		donk_wavelet_transform(tcb, 16);
		
		
		int luma_cutoff_a = donk_wavelet_cutoff(tluma, 256, donk_wavelet_pattern_a, 12, 32);
		int luma_cutoff_b = donk_wavelet_cutoff(tluma, 256, donk_wavelet_pattern_b, 12, 32);
		int luma_cutoff_c = donk_wavelet_cutoff(tluma, 256, donk_wavelet_pattern_c, 12, 32);
		int luma_cutoff_d = donk_wavelet_cutoff(tluma, 256, donk_wavelet_pattern_d, 12, 32);
		unsigned char wavelet_pattern = 0;
		int luma_cutoff = luma_cutoff_a;
		if (luma_cutoff_b < luma_cutoff) luma_cutoff = luma_cutoff_b, wavelet_pattern = 1;
		if (luma_cutoff_c < luma_cutoff) luma_cutoff = luma_cutoff_c, wavelet_pattern = 2;
		if (luma_cutoff_d < luma_cutoff) luma_cutoff = luma_cutoff_d, wavelet_pattern = 3;
		
		int chroma_cutoff_cr = donk_wavelet_cutoff(tcr, 256, donk_wavelet_pattern_d, 12, 32);
		int chroma_cutoff_cb = donk_wavelet_cutoff(tcb, 256, donk_wavelet_pattern_d, 12, 32);
		int chroma_cutoff = chroma_cutoff_cr > chroma_cutoff_cb ? chroma_cutoff_cr : chroma_cutoff_cb;
		if (chroma_cutoff >= 15) chroma_cutoff = 15;
		
		for (int i = 0; i < 256; i++) {
			if (tcr[i] < -128) tcr[i] = -128;
			if (tcr[i] > 127) tcr[i] = 127;
			if (tcb[i] < -128) tcb[i] = -128;
			if (tcb[i] > 127) tcb[i] = 127;
			
			if (i == 0) continue;
			
			if (tluma[i] < -128) tluma[i] = -128;
			if (tluma[i] > 127) tluma[i] = 127;
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
			packed_luma[i] = tluma[luma_pattern[i]];
			packed_cr[i] = tcr[donk_wavelet_pattern_d[i]];
			packed_cb[i] = tcb[donk_wavelet_pattern_d[i]];
		}
		
		unsigned char quantized_luma[256];
		unsigned char quantized_cr[256];
		unsigned char quantized_cb[256];
		
		unsigned char luma_bits = 6;
		unsigned char chroma_bits = 6;
		
		
		donk_quantize(packed_luma, luma_cutoff, luma_bits, quantized_luma);
		donk_quantize(packed_cr, chroma_cutoff, chroma_bits, quantized_cr);
		donk_quantize(packed_cb, chroma_cutoff, chroma_bits, quantized_cb);
		int luma_bytes = donk_quantized_bytes(luma_cutoff, luma_bits);
		int chroma_bytes = donk_quantized_bytes(chroma_cutoff, chroma_bits);
		
		unsigned char w_cutoff = luma_cutoff;
		unsigned char chroma_pack = wavelet_pattern | (((unsigned char)chroma_cutoff) << 2);
		unsigned char quantization = luma_bits << 4 | chroma_bits;
		
		donk_write_bytes(encoder->stream, 1, &w_cutoff);
		donk_write_bytes(encoder->stream, 1, &chroma_pack);
		donk_write_bytes(encoder->stream, 1, &quantization);
		donk_write_bytes(encoder->stream, luma_bytes, quantized_luma);
		
		donk_write_bytes(encoder->stream, chroma_bytes, quantized_cr);
		donk_write_bytes(encoder->stream, chroma_bytes, quantized_cb);
	}
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
	
	int horizontal_blocks = (header.width >> 4) << 4 == header.width 
		? header.width : ((header.width >> 4) + 1) << 4;
	int vertical_blocks = (header.width >> 4) << 4 == header.width 
		? header.width : ((header.width >> 4) + 1) << 4;
	for (int h = 0; h < horizontal_blocks; h += 16)
	for (int w = 0; w < vertical_blocks; w += 16) {
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
		
		int tluma[16 * 16];
		int tcr[16 * 16];
		int tcb[16 * 16];
		
		memset(tluma, 0, 16 * 16 * 4);
		memset(tcr, 0, 16 * 16 * 4);
		memset(tcb, 0, 16 * 16 * 4);
		
		tluma[0] = (unsigned char)packed_luma[0];
		tcr[0] = packed_cr[0];
		tcb[0] = packed_cb[0];
		
		for (int i = 1; i < 256; i++) {
			tluma[luma_pattern[i]] = packed_luma[i];
			tcr[donk_wavelet_pattern_d[i]] = packed_cr[i];
			tcb[donk_wavelet_pattern_d[i]] = packed_cb[i];
		}
		
		donk_inv_wavelet_transform(tluma, 16);
		donk_inv_wavelet_transform(tcr, 16);
		donk_inv_wavelet_transform(tcb, 16);
		
		int w_extent = w + 16 > header.width ? header.width : w + 16;
		int h_extent = h + 16 > header.height ? header.height : h + 16;
		
		for (int y = h; y < h_extent; y++)
		for (int x = w; x < w_extent; x++) {
			int destination_offset = (y - h) * 16 + x - w;
			int luma = tluma[destination_offset];
			int cr = tcr[destination_offset];
			int cb = tcb[destination_offset];
			
			int r = donk_r_from_ycrcb(cr, cb, luma);
			int b = donk_b_from_ycrcb(cr, cb, luma);

			if (r < 0) r = 0;
			if (r > 255) r = 255;
			if (b < 0) b = 0;
			if (b > 255) b = 255;
			
			int g = donk_g_from_yrb(r, b, luma);
			
			if (g < 0) g = 0;
			if (g > 255) g = 255;
			
			int source_offset = 3 * (y * header.width + x);
			frame->image.pixels[source_offset + 0] = r;
			frame->image.pixels[source_offset + 1] = g;
			frame->image.pixels[source_offset + 2] = b;
		}	
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



