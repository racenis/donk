#include "memory.h"

/*
	TODO: FIGURE OUT AN ARENA SCHEME

	Calling malloc/free repeatedly during decoding, e.g. for each 4x4 pixel
	block could negatively impact performance. It would be beneficial if we had
	some kind of an arena allocator or something like it.
	
	This would probably involve some experimentation, so we'll do it after the
	encoder and decoder are at least partially implemented.

	TODO: ALLOW USER TO SET CUSTOM ALLOCATOR
	
*/

char* donk_malloc(int bytes) {
	return malloc(bytes);
}

void donk_free(char* memory) {
	free(memory);
}