#ifndef VECTOR_H
#define VECTOR_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct vector;

struct vector *vector_create(size_t init_capacity, size_t el_size);
void vector_free(struct vector *);

size_t vec_get_len(const struct vector *vec);
void *vector_get_el(struct vector *vec, size_t index);
int vector_add_el(struct vector *vec, void *element, size_t el_size);
int vector_del_el(struct vector *vec, size_t idx);

#endif
