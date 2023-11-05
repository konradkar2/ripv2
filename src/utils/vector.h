#ifndef VECTOR_H
#define VECTOR_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

struct vector;

struct vector *vector_create(size_t init_capacity, size_t el_size);
void vector_free(struct vector *);

size_t vector_get_len(const struct vector *vec);
void *vector_get(struct vector *vec, size_t index);
ssize_t vector_find(struct vector *vec, void *filter_entry,
			bool(filter_func)(void *entry, void *filter_entry));
int vector_add(struct vector *vec, void *element, size_t el_size);
int vector_del(struct vector *vec, size_t idx);

#endif
