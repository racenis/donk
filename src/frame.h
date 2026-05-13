#ifndef DONK_FRAME_H
#define DONK_FRAME_H

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

#endif // DONK_FRAME_H