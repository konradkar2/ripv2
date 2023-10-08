#ifndef VECTOR_H
#define VECTOR_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct vector {
	void *data;
	size_t length;
	size_t total_length;
	size_t el_size;
} vector;

void vector_init(vector *vec, size_t total_length, void *data, size_t el_size);
size_t vec_get_len(vector *vec);
size_t vec_get_len_total(vector *vec);
size_t vec_get_len_left(vector *vec);
void *vector_get_el(vector *vec, size_t index);
int vector_add_el(vector *vec, void *element, size_t el_size);
int vector_del_el(vector *vec, size_t idx);

#endif