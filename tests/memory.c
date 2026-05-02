#define UTEST_IMPLEMENTATION
#include "utest.h"

#include "memory.h"

UTEST(memory, default_malloc_delete) {
	char* data = donk_malloc(23);
	
	ASSERT_NE(data, NULL);
	
	// these shouldn't crash
	data[0] = 'A';
	data[22] = 'B';
	
	donk_free(data);
	
	ASSERT_TRUE(1);
}

UTEST_MAIN()