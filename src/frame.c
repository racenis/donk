#include "frame.h"

#include <string.h>
#include <stdlib.h>

/*
	TODO: IMPLEMENT FRAME DECODING AND ENCODING
	
	Currently we are planning for these types of frames:
	- audio
	- image wavelet
	- delta wavelet
	- image palette
	- delta palette
	- audio
	
	Maybe we will also have some stream metadata and coding tables.
*/




void donk_frame_image_make(donk_frame_t* frame, int width, int height) {
	memset(frame, 0, sizeof(donk_frame_t));
	frame->type = DONK_FRAME_IMAGE;
	frame->image.width = width;
	frame->image.height = height;
	frame->image.pixels = malloc(width * height * 3);
}

void donk_frame_yeet(donk_frame_t* frame) {
	switch (frame->type) {
		DONK_FRAME_IMAGE:
			free(frame->image.pixels);
			break;
		DONK_FRAME_AUDIO:
			break;
	}
	
	memset(frame, 0, sizeof(donk_frame_t));
} 