#ifndef DONK_ARRAY_H
#define DONK_ARRAY_H

typedef struct {
	char* begin;
	char* end;
	int element_size;
	int element_count;
	int allocated_elements;
} donk_array_t;

void donk_array_init(donk_array_t* array, int element_size, int initial_count);
void donk_array_yeet(donk_array_t* array);
void donk_array_append(donk_array_t* array, const void* element);

#define donk_array_get(ARRAY, INDEX, TYPE) \
	*(TYPE*)((ARRAY)->begin + (sizeof(TYPE) * INDEX))
	
#define donk_array_set(ARRAY, INDEX, TYPE, VALUE) \
	*(TYPE*)((ARRAY)->begin + (sizeof(TYPE) * INDEX)) = VALUE

#define donk_array_first(ARRAY, TYPE) \
	*(TYPE*)((ARRAY)->begin)
	
#define donk_array_last(ARRAY, TYPE) \
	*(TYPE*)((ARRAY)->end - sizeof(TYPE))

#define donk_array_for_each(ARRAY, IT, TYPE) \
	for (TYPE* IT = (TYPE*)((ARRAY)->begin); IT < (TYPE*)(ARRAY)->end; IT++)

void donk_array_sort(donk_array_t* array, int (compare)(char*, char*));

#endif // DONK_ARRAY_H