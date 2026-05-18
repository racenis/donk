#include "frame.h"

#include "coding.h"

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

void donk_palette_make(donk_palette_t* palette) {
	memset(palette, 0, sizeof(donk_palette_t));
	donk_array_init(&palette->subpalette_stats, 16 * 2, 0);
}

void donk_palette_yeet(donk_palette_t* palette) {
	donk_array_yeet(&palette->subpalette_stats);
	if (palette->colors) free(palette->colors);
	if (palette->subpalettes) free(palette->subpalettes);
	memset(palette, 0, sizeof(donk_palette_t));
}

void donk_palette_add_color(donk_palette_t* palette, unsigned char r, unsigned char g, unsigned char b) {
	if (!palette->stats) {
		palette->stats = calloc(65536, 4);
	}
	
	// quantizing a full color from 24 to 16 bits allows us to reduce memory
	// consumption from 64MB to 256KB -- important if we want to encode images
	// on a literal toaster
	unsigned short key = donk_color_to_key(r, g, b);
	palette->stats[key]++;
}

void donk_palette_build(donk_palette_t* palette) {
	struct {
		unsigned char r; unsigned char g; unsigned char b;
		long rsum; long gsum; long bsum; long sumc;
	} clusters[256];
	memset(clusters, 0, sizeof(clusters));
	
	// init random colors
	for (int i = 0; i < 256; i++) {
		short color = i * 256; // pick arbitrary color
		
		// find next color that is in set
		while (!palette->stats[color]) color++;
		
		clusters[i].r = donk_key_to_r(color);
		clusters[i].g = donk_key_to_g(color);
		clusters[i].b = donk_key_to_b(color);
	}
	
	// very inefficient k-means algorithm
	for (int k = 0; k < 5; k++) {
		for (int c = 0; c < 65536; c++) {
			const int r = donk_key_to_r(c);
			const int g = donk_key_to_g(c);
			const int b = donk_key_to_b(c);
			
			unsigned int closest_distance = ~0;
			int closest = -1;
			
			for (int i = 0; i < 256; i++) {
				int r_dif = r - (int)clusters[i].r;
				int g_dif = g - (int)clusters[i].g;
				int b_dif = b - (int)clusters[i].b;
				
				if (r_dif < 0) r_dif = -r_dif;
				if (g_dif < 0) g_dif = -g_dif;
				if (b_dif < 0) b_dif = -b_dif;
				
				const int distance = r_dif + g_dif + b_dif;
				if (distance < closest_distance) {
					closest_distance = distance;
					closest = i;
				}
			}
			
			clusters[closest].rsum += palette->stats[c] * r;
			clusters[closest].gsum += palette->stats[c] * g;
			clusters[closest].bsum += palette->stats[c] * b;
			
			clusters[closest].sumc += palette->stats[c];
		}
		
		for (int i = 0; i < 256; i++) {
			if (!clusters[i].sumc) continue;
			
			clusters[i].r = clusters[i].rsum / clusters[i].sumc;
			clusters[i].g = clusters[i].gsum / clusters[i].sumc;
			clusters[i].b = clusters[i].bsum / clusters[i].sumc;
			
			clusters[i].rsum = 0;
			clusters[i].gsum = 0;
			clusters[i].bsum = 0;
			clusters[i].sumc = 0;
		}
	}
	
	palette->colors = malloc(256 * 2);
	for (int i = 0; i < 256; i++) {
		palette->colors[i] = donk_color_to_key(clusters[i].r, clusters[i].g, clusters[i].b);
	}
}

static unsigned int color_dist(unsigned short key, unsigned char r, unsigned char g, unsigned char b) {
	const int kr = donk_key_to_r(key);
	const int kg = donk_key_to_g(key);
	const int kb = donk_key_to_b(key);
	
	int r_dif = kr - (int)r;
	int g_dif = kg - (int)g;
	int b_dif = kb - (int)b;

	if (r_dif < 0) r_dif = -r_dif;
	if (g_dif < 0) g_dif = -g_dif;
	if (b_dif < 0) b_dif = -b_dif;

	return r_dif + g_dif + b_dif;
}

void donk_palette_add_block(donk_palette_t* palette, unsigned char* r, unsigned char* g, unsigned char* b) {
	int color_stats[256];
	memset(color_stats, 0, sizeof(color_stats));
	
	// find frequency of palette colors in block
	for (int i = 0; i < 256; i++) {
		unsigned int best_distance = ~0;
		int best_color = -1;
		
		for (int c = 0; c < 256; c++) {
			unsigned int dist = color_dist(palette->colors[c], r[i], g[i], b[i]);
			
			if (dist < best_distance) {
				best_distance = dist;
				best_color = c;
			}
		}
		
		color_stats[best_color]++;
	}
	
	// find 16 most frequent palette colors
	short colors[16];
	for (int i = 0; i < 16; i++) {
		short commonest = -1;
		int commonestc = 0;
		
		for (unsigned char j = 0; j < 256; j++) {
			if (color_stats[j] > commonestc) {
				commonestc = color_stats[j];
				commonest = j;
			}
		}
		
		colors[i] = commonest;
		if (commonest > 0) color_stats[commonest] = 0;
	}
	
	donk_array_append(&palette->subpalette_stats, colors);
}

void donk_palette_build_subpalettes(donk_palette_t* palette) {
	struct {
		int colors[256];
		int frequency[256];
	} subpalettes[32];
	memset(subpalettes, 0, sizeof(subpalettes));
	
	for (int i = 0; i < 32; i++) {
		for (int j = i * 8; j < i * 8 + 8; j++) {
			subpalettes[i].colors[j] = 1;
		}
	}
	
	for (int k = 0; k < 5; k++) {
		for (int p = 0; p < palette->subpalette_stats.element_count; p++) {
			char* offset = palette->subpalette_stats.begin + p * 2 * 16;
			short* pal = (short*)offset;
			
			
			int best_palette = 0;
			int best_match = 0;
			
			for (int s = 0; s < 32; s++) {
				int match = 0;
				for (int i = 0; i < 16; i++) {
					if (pal[i] < 0) continue;
					if (subpalettes[s].colors[pal[i]]) match++;
				}
				if (match > best_match) {
					best_match = match;
					best_palette = s;
				}
			}
			
			for (int i = 0; i < 16; i++) {
				if (pal[i] < 0) continue;
				subpalettes[best_palette].frequency[pal[i]]++;
			}
		}
		
		for (int s = 0; s < 32; s++) {
			memset(subpalettes[s].colors, 0, sizeof(subpalettes[s].colors));
			for (;;) {
				int colors_represented = 0;
				int least_frequent = 0;
				int least_frequency = INT_MAX;
				for (int i = 0; i < 256; i++) {
					if (subpalettes[s].frequency[i] > 0) colors_represented++;
					if (subpalettes[s].frequency[i] < least_frequency) {
						least_frequency = subpalettes[s].frequency[i];
						least_frequent = i;
					}
				}
				
				if (colors_represented <= 16) break;
				
				subpalettes[s].frequency[least_frequent] = 0;
			}
			
			for (int i = 0; i < 256; i++) {
				if (subpalettes[s].frequency[i]) {
					subpalettes[s].colors[i] = 1;
				}
				subpalettes[s].frequency[i] = 0;
			}
		}
	}
	
	palette->subpalettes = malloc(32 * 16);
	for (int s = 0; s < 32; s++) {
		unsigned char colors[16];
		memset(colors, 0, 16);
		int last_color = 0;
		for (unsigned char i = 0; i < 256; i++) {
			if (subpalettes[s].colors[i]) {
				colors[last_color++] = i;
			}
		}
		for (int i = 0; i < 16; i++) {
			palette->subpalettes[s * 16 + i] = colors[i];
		}
	}
}

int donk_palette_best_subpalette(donk_palette_t* palette, unsigned char* r, unsigned char* g, unsigned char* b) {
	int best_palette = 0;
	int best_distance = INT_MAX;
	for (int s = 0; s < 32; s++) {
		int distance = 0;
		
		for (int i = 0; i < 256; i++) {
			int lowest_dist = INT_MAX;
			for (int c = 0; c < 16; c++) {
				int this_dist = color_dist(palette->colors[palette->subpalettes[s * 16 + c]], r[i], g[i], b[i]);
				if (this_dist < lowest_dist) lowest_dist = this_dist;
			}
			distance += lowest_dist;
		}
		
		if (distance < best_distance) {
			best_distance = distance;
			best_palette = s;
		}
	}
	
	return best_palette;
}

void donk_palette_subpalettize(donk_palette_t* palette, int subpalette, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* res) {
	for (int i = 0; i < 256; i++) {
		int best_color = 0;
		int best_distance = INT_MAX;
		for (int c = 0; c < 16; c++) {
			int distance = color_dist(palette->colors[palette->subpalettes[subpalette * 16 + c]], r[i], g[i], b[i]);
			if (distance < best_distance) {
				best_distance = distance;
				best_color = c;
			}
		}
		res[i] = best_color;
	}
}
