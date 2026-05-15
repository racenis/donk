#include "coding.h"

/*
	TODO: IMPLEMENT HUFFMAN CODING
	
	We'll need build up dictionaries, encode and decode.
	
	TODO: IMPLEMENT WAVELET TRANSFORM
	
	For removing high frequency detail.
	
	TODO: 
		- palette generation stuff 
		- ?? idk
*/

static void transform_1d(int* array, int size) {
	int* low = alloca(sizeof(int) * size/2);
	int* high = alloca(sizeof(int) * size/2);
	
	for (int i = 0; i < size/2; i++) {
		low[i] = (array[i * 2] + array[i * 2 + 1]) / 2;
		high[i] = (array[i * 2] - array[i * 2 + 1]) / 2;
	}
	
	if (size >= 4) {
		transform_1d(low, size/2);
	}
	
	for (int i = 0; i < size/2; i++) {
		array[i] = low[i];
		array[i + size / 2] = high[i];
	}
}

void donk_wavelet_transform(int* array, int size) {
	int* row = alloca(sizeof(int) * size);
	
	for (int y = 0; y < size; y++) {
		transform_1d(array + size * y, size);
	}
	
	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			row[y] = array[y * size + x];
		}
		transform_1d(row, size);
		for (int y = 0; y < size; y++) {
			array[y * size + x] = row[y];
		}
	}
}



static void inv_transform_1d(int* array, int size) {
	int* low = alloca(sizeof(int) * size/2);
	int* high = alloca(sizeof(int) * size/2);
	
	for (int i = 0; i < size/2; i++) {
		low[i] = array[i];
		high[i] = array[i + size / 2];
	}
	
	if (size >= 4) {
		inv_transform_1d(low, size/2);
	}
	
	for (int i = 0; i < size/2; i++) {
		array[i * 2] = low[i] + high[i];
		array[i * 2 + 1] = low[i] - high[i];
	}
}

void donk_inv_wavelet_transform(int* array, int size) {
	int* row = alloca(sizeof(int) * size);
	
	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			row[y] = array[y * size + x];
		}
		inv_transform_1d(row, size);
		for (int y = 0; y < size; y++) {
			array[y * size + x] = row[y];
		}
	}
	
	for (int y = 0; y < size; y++) {
		inv_transform_1d(array + size * y, size);
	}
}

int donk_wavelet_cutoff(int* array, int size, const int* lut, int quality, int threshold) {
	int total_value = 0;
	for (int i = 0; i < size; i++) {
		int value = array[lut[i]];
		if (value < 0) value = -value;
		total_value += value;
	}
	
	if (total_value == 0) return 1;
	
	int window_value = 0;
	for (int i = 0; i < 8; i++) {
		int value = array[lut[i]];
		if (value < 0) value = -value;
		window_value += value;
	}
	
	int current_value = 0;
	for (int i = 0; i < size - 8; i++) {
		int value = array[lut[i]];
		if (value < 0) value = -value;
		current_value += value;
		
		if ((current_value << 4) / total_value >= quality) {
			return i + 1;
		}
		
		int next_value = array[lut[i]];
		if (next_value < 0) next_value = -next_value;
		window_value -= value;
		window_value += next_value;
		
		if (window_value < threshold) {
			return i + 1;
		}
	}
	
	return size - 1;
}

void donk_quantize(unsigned char* unpacked, int size, int bits, unsigned char* packed) {
	memset(packed, 0, size);
	int inv_bits = 8 - bits;
	int bits_packed = 0;
	for (int i = 0; i < size; i++) {
		unsigned char value = unpacked[i] >> inv_bits;
		int byte = bits_packed >> 3;
		int bit_in_byte = bits_packed - (byte << 3);
		int bits_left = 8 - bit_in_byte;
		if (bits_left >= bits) {
			packed[byte] |= (value << bit_in_byte);
		} else {
			packed[byte] |= (value << bit_in_byte);
			packed[byte + 1] |= (value >> bits_left);
		}
		bits_packed += bits;
	}
}

void donk_dequantize(unsigned char* unpacked, int size, int bits, unsigned char* packed) {
	int inv_bits = 8 - bits;
	int bits_unpacked = 0;
	for (int i = 0; i < size; i++) {
		int byte = bits_unpacked >> 3;
		int bit_in_byte = bits_unpacked - (byte << 3);
		int bits_left = 8 - bit_in_byte;
		unsigned char value;
		if (bits_left >= bits) {
			value = (packed[byte] >> bit_in_byte);
		} else {
			value = packed[byte] >> bit_in_byte;
			value |= (packed[byte + 1] << bits_left);
		}
		unpacked[i] = value << inv_bits;
		bits_unpacked += bits;
	}
}

int donk_quantized_bytes(int size, int bits) {
	return ((size) * (bits) + 7) / 8;
}

const int donk_wavelet_pattern_a[256] = {
	0, 16, 1, 2, 17, 32, 48, 33, 18, 3, 4, 19, 34, 49, 64, 80,
	65, 50, 35, 20, 5, 6, 21, 36, 51, 66, 81, 96, 112, 97, 82, 67,
	52, 37, 22, 7, 8, 23, 38, 53, 68, 83, 98, 113, 128, 144, 129, 114,
	99, 84, 69, 54, 39, 24, 9, 10, 25, 40, 55, 70, 85, 100, 115, 130,
	145, 160, 176, 161, 146, 131, 116, 101, 86, 71, 56, 41, 26, 11, 12, 27,
	42, 57, 72, 87, 102, 117, 132, 147, 162, 177, 192, 208, 193, 178, 163, 148,
	133, 118, 103, 88, 73, 58, 43, 28, 13, 14, 29, 44, 59, 74, 89, 104,
	119, 134, 149, 164, 179, 194, 209, 224, 240, 225, 210, 195, 180, 165, 150, 135,
	120, 105, 90, 75, 60, 45, 30, 15, 31, 46, 61, 76, 91, 106, 121, 136,
	151, 166, 181, 196, 211, 226, 241, 242, 227, 212, 197, 182, 167, 152, 137, 122,
	107, 92, 77, 62, 47, 63, 78, 93, 108, 123, 138, 153, 168, 183, 198, 213,
	228, 243, 244, 229, 214, 199, 184, 169, 154, 139, 124, 109, 94, 79, 95, 110,
	125, 140, 155, 170, 185, 200, 215, 230, 245, 246, 231, 216, 201, 186, 171, 156,
	141, 126, 111, 127, 142, 157, 172, 187, 202, 217, 232, 247, 248, 233, 218, 203,
	188, 173, 158, 143, 159, 174, 189, 204, 219, 234, 249, 250, 235, 220, 205, 190,
	175, 191, 206, 221, 236, 251, 252, 237, 222, 207, 223, 238, 253, 254, 239, 255
};

const int donk_wavelet_pattern_b[256] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

const int donk_wavelet_pattern_c[256] = {
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240,
	1, 17, 33, 49, 65, 81, 97, 113, 129, 145, 161, 177, 193, 209, 225, 241,
	2, 18, 34, 50, 66, 82, 98, 114, 130, 146, 162, 178, 194, 210, 226, 242,
	3, 19, 35, 51, 67, 83, 99, 115, 131, 147, 163, 179, 195, 211, 227, 243,
	4, 20, 36, 52, 68, 84, 100, 116, 132, 148, 164, 180, 196, 212, 228, 244,
	5, 21, 37, 53, 69, 85, 101, 117, 133, 149, 165, 181, 197, 213, 229, 245,
	6, 22, 38, 54, 70, 86, 102, 118, 134, 150, 166, 182, 198, 214, 230, 246,
	7, 23, 39, 55, 71, 87, 103, 119, 135, 151, 167, 183, 199, 215, 231, 247,
	8, 24, 40, 56, 72, 88, 104, 120, 136, 152, 168, 184, 200, 216, 232, 248,
	9, 25, 41, 57, 73, 89, 105, 121, 137, 153, 169, 185, 201, 217, 233, 249,
	10, 26, 42, 58, 74, 90, 106, 122, 138, 154, 170, 186, 202, 218, 234, 250,
	11, 27, 43, 59, 75, 91, 107, 123, 139, 155, 171, 187, 203, 219, 235, 251,
	12, 28, 44, 60, 76, 92, 108, 124, 140, 156, 172, 188, 204, 220, 236, 252,
	13, 29, 45, 61, 77, 93, 109, 125, 141, 157, 173, 189, 205, 221, 237, 253,
	14, 30, 46, 62, 78, 94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 254,
	15, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 207, 223, 239, 255
};

const int donk_wavelet_pattern_d[256] = {
	0, 1, 16, 2, 32, 3, 48, 4, 64, 5, 80, 6, 96, 7, 112, 8,
	128, 9, 144, 10, 160, 11, 176, 12, 192, 13, 208, 14, 224, 15, 240, 17,
	18, 33, 19, 49, 20, 65, 21, 81, 22, 97, 23, 113, 24, 129, 25, 145,
	26, 161, 27, 177, 28, 193, 29, 209, 30, 225, 31, 241, 34, 35, 50, 36,
	51, 66, 37, 52, 67, 82, 38, 53, 68, 83, 98, 39, 54, 69, 84, 99,
	114, 40, 55, 70, 85, 100, 115, 130, 41, 56, 71, 86, 101, 116, 131, 146,
	42, 57, 72, 87, 102, 117, 132, 147, 162, 43, 58, 73, 88, 103, 118, 133,
	148, 163, 178, 44, 59, 74, 89, 104, 119, 134, 149, 164, 179, 194, 45, 60,
	75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 46, 61, 76, 91, 106, 121,
	136, 151, 166, 181, 196, 211, 226, 47, 62, 77, 92, 107, 122, 137, 152, 167,
	182, 197, 212, 227, 242, 63, 78, 93, 108, 123, 138, 153, 168, 183, 198, 213,
	228, 243, 79, 94, 109, 124, 139, 154, 169, 184, 199, 214, 229, 244, 95, 110,
	125, 140, 155, 170, 185, 200, 215, 230, 245, 111, 126, 141, 156, 171, 186, 201,
	216, 231, 246, 127, 142, 157, 172, 187, 202, 217, 232, 247, 143, 158, 173, 188,
	203, 218, 233, 248, 159, 174, 189, 204, 219, 234, 249, 175, 190, 205, 220, 235,
	250, 191, 206, 221, 236, 251, 207, 222, 237, 252, 223, 238, 253, 239, 254, 255
};