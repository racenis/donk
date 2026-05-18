#ifndef DONK_CODING_H
#define DONK_CODING_H

#define donk_luma_from_rgb(R, G, B) ((R >> 2) + (G >> 1) + (G >> 3) + (B >> 3))

#define donk_chroma_from_ry(R, Y) (R - Y)
#define donk_chroma_from_by(B, Y) (B - Y)

#define donk_color_clamp(C) (C > 255 ? 255 : C < 0 ? 0 : C)
#define donk_color_compress(C) (C < 128 ? C + 16 : C)

#define donk_r_from_ycrcb(CR, CB, Y) (CR + Y)
#define donk_b_from_ycrcb(CR, CB, Y) (CB + Y)
#define donk_g_from_yrb(R, B, Y) (Y - (B >> 7) - (R >> 7))

#define donk_color_to_key(R, G, B) (((unsigned short)R & 0xF8) << 8) \
	| (((unsigned short)G & 0xFC) << 3) | ((unsigned short)B >> 3)
#define donk_key_to_r(K) (((K >> 11) & 0x1F) << 3)
#define donk_key_to_g(K) (((K >> 5) & 0x3F) << 2)
#define donk_key_to_b(K) ((K & 0x1F) <<3)

void donk_wavelet_transform(int* array, int size);
void donk_inv_wavelet_transform(int* array, int size);

int donk_wavelet_cutoff(int* array, int size, const int* lut, int quality, int threshold);

extern const int donk_wavelet_pattern_a[256];
extern const int donk_wavelet_pattern_b[256];
extern const int donk_wavelet_pattern_c[256];
extern const int donk_wavelet_pattern_d[256];

void donk_quantize(unsigned char* unpacked, int size, int bits, unsigned char* packed);
void donk_dequantize(unsigned char* unpacked, int size, int bits, unsigned char* packed);
int donk_quantized_bytes(int size, int bits);

int donk_quantize_bits(unsigned char* array, int size, int quality);

#endif // DONK_CODING_H