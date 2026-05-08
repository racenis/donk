#include "array.h"

#include "memory.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void donk_array_init(donk_array_t* array, int element_size, int initial_count) {
	memset(array, 0, sizeof(donk_array_t));
	if (initial_count < 0) initial_count = 1;
	assert(element_size > 0);
	array->begin = donk_malloc(element_size * initial_count);
	array->end = array->begin;
	array->element_size = element_size;
	array->element_count = initial_count;
	array->allocated_elements = initial_count;
}

void donk_array_yeet(donk_array_t* array) {
	donk_free(array->begin);
	memset(array, 0, sizeof(donk_array_t));
}

static void resize_array(donk_array_t* array) {
	// grow array by 1.5x
	int new_allocated_elements = array->allocated_elements >> 1;
	if (new_allocated_elements <= 0) new_allocated_elements = 1;
	array->allocated_elements += new_allocated_elements;
	// renew pointers
	const int new_bytes = array->allocated_elements * array->element_size;
	array->begin = donk_realloc(array->begin, new_bytes);
	array->end = array->begin + array->element_size * array->element_count;
}

void donk_array_append(donk_array_t* array, const void* element) {
	if (array->element_count >= array->allocated_elements) resize_array(array);
	memcpy(array->end, element, array->element_size);
	array->end += array->element_size;
	array->element_count++;
}

static void swap_elements(char* array, int element_size, int a, int b) {
	a *= element_size;
	b *= element_size;
	for (int i = 0; i < element_size; i++) {
		char byte = array[a];
		array[a] = array[b];
		array[b] = byte;
		a++; b++;
	}
}

static void sort_recursive(char* array, int (compare)(char*, char*), int element_size, int left, int right) {
	if (left >= right) return;
	
	char* pivot = alloca(element_size);
	memcpy(pivot, array + left * element_size, element_size);
	
	int p1 = left - 1;
	int p2 = right + 1;
	
	for (;;) {
		do p1++; while (compare(array + p1 * element_size, pivot) < 0);
		do p2--; while (compare(array + p2 * element_size, pivot) > 0);
		if (p1 >= p2) break;
		swap_elements(array, element_size, p1, p2);
	}
	
	sort_recursive(array, compare, element_size, left, p2);
	sort_recursive(array, compare, element_size, p2 + 1, right);
}

void donk_array_sort(donk_array_t* array, int (compare)(char*, char*)) {
	sort_recursive(array->begin, compare, array->element_size, 0, array->element_count - 1);
}