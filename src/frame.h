#ifndef DONK_FRAME_H
#define DONK_FRAME_H

#include "array.h"

enum {
	DONK_FRAME_IMAGE,
	DONK_FRAME_AUDIO
};

typedef struct {
	int type;
	
	int channel;
	int timestamp;
	
	union {
		struct {
			short* samples;
			int sample_count;
		} audio;
		struct {
			int width;
			int height;
			
			unsigned char* pixels;
		} image;
	};
} donk_frame_t;

void donk_frame_audio_make(donk_frame_t* frame, int samples); 
void donk_frame_image_make(donk_frame_t* frame, int width, int height); 

void donk_frame_yeet(donk_frame_t* frame); 

typedef struct {
	int* stats;
	unsigned short* colors;
	unsigned char* subpalettes;
	int subpalette_count;
	
	donk_array_t subpalette_stats;
} donk_palette_t;

void donk_palette_make(donk_palette_t* palette);
void donk_palette_add_color(donk_palette_t* palette, unsigned char r, unsigned char g, unsigned char b);
void donk_palette_build(donk_palette_t* palette);
void donk_palette_add_block(donk_palette_t* palette, unsigned char* r, unsigned char* g, unsigned char* b);
void donk_palette_build_subpalettes(donk_palette_t* palette);
int donk_palette_best_subpalette(donk_palette_t* palette, unsigned char* r, unsigned char* g, unsigned char* b);
void donk_palette_subpalettize(donk_palette_t* palette, int subpalette, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* res);
void donk_palette_yeet(donk_palette_t* palette);

#endif // DONK_FRAME_H