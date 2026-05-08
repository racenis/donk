#ifndef DONK_MEMORY_H
#define DONK_MEMORY_H

char* donk_malloc(int);
void donk_free(char*);

char* donk_realloc(char*, int);

#endif // DONK_MEMORY_H