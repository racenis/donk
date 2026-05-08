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

UTEST(memory, default_realloc) {
	char* data = donk_malloc(2);
	
	ASSERT_NE(data, NULL);
	
	// random data
	data[0] = '6';
	data[1] = '7';
	
	ASSERT_EQ(data[0], '6');
	ASSERT_EQ(data[1], '7');
	
	data = donk_realloc(data, 10000);
	
	// check that didn't change
	ASSERT_EQ(data[0], '6');
	ASSERT_EQ(data[1], '7');
	
	donk_free(data);
	
	ASSERT_TRUE(1);
}

UTEST_MAIN()