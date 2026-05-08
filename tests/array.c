#define UTEST_IMPLEMENTATION
#include "utest.h"

#include "array.h"

UTEST(array, basics) {
	donk_array_t array;
	
	donk_array_init(&array, sizeof(int), 4);
	
	donk_array_set(&array, 2, int, 67);
	ASSERT_EQ(donk_array_get(&array, 2, int), 67);
	
	donk_array_yeet(&array);
	
	ASSERT_TRUE(1);
}

UTEST(array, zero_element) {
	donk_array_t array;
	
	donk_array_init(&array, sizeof(int), 0);
	
	const int num1 = 67;
	const int num2 = 23;
	const int num3 = 420;
	
	donk_array_append(&array, &num1);
	donk_array_append(&array, &num2);
	donk_array_append(&array, &num3);
	
	ASSERT_EQ(donk_array_get(&array, 0, int), num1);
	ASSERT_EQ(donk_array_get(&array, 1, int), num2);
	ASSERT_EQ(donk_array_get(&array, 2, int), num3);
	
	donk_array_yeet(&array);
	
	ASSERT_TRUE(1);
}

UTEST(array, first_last) {
	donk_array_t array;
	
	donk_array_init(&array, sizeof(int), 0);
	
	const int num = 67;
	
	donk_array_append(&array, &num);

	ASSERT_EQ(donk_array_first(&array, int), num);
	ASSERT_EQ(donk_array_last(&array, int), num);
	
	donk_array_yeet(&array);
	
	ASSERT_TRUE(1);
}

UTEST(array, large_array) {
	donk_array_t array;
	
	donk_array_init(&array, sizeof(int), 0);
	
	for (int i = 0; i < 1024; i++) {
		donk_array_append(&array, &i);
	}
	
	int count = 0;
	donk_array_for_each(&array, it, int) {
		ASSERT_EQ(*it, count);
		count++;
	}
	
	ASSERT_EQ(count, 1024);
	
	donk_array_yeet(&array);
	
	ASSERT_TRUE(1);
}

static int (int_comparison)(char* a, char* b) {
	return *(int*)a - *(int*)b;
}

UTEST(array, sort) {
	donk_array_t array;
	
	donk_array_init(&array, sizeof(int), 7);
	
	int values_initial[7] = {1, 69, 23, -4, 420, 67, 0};
	int values_sorted[7] = {-4, 0, 1, 23, 67, 69, 420};
	
	for (int i = 0; i < 7; i++) {
		donk_array_set(&array, i, int, values_initial[i]);
	}
	
	donk_array_sort(&array, int_comparison);
	
	for (int i = 0; i < 7; i++) {
		ASSERT_EQ(donk_array_get(&array, i, int), values_sorted[i]);
	}
	
	donk_array_yeet(&array);
	
	ASSERT_TRUE(1);
}

UTEST_MAIN()