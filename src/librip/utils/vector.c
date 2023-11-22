#include "vector.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

struct vector {
	void *data;
	size_t length;
	size_t capacity;
	size_t init_capacity;
	size_t el_size;
};

struct vector *vector_create(size_t init_capacity, size_t el_size)
{
	assert("invalid vector data" && init_capacity > 0);
	assert("invalid vector data" && el_size > 0);

	struct vector *vec = NULL;

	vec = calloc(1, sizeof(struct vector));
	if (!vec) {
		return NULL;
	}

	vec->data = calloc(1, init_capacity * el_size);

	if (!vec->data) {
		vector_free(vec);
		return NULL;
	}

	vec->init_capacity = init_capacity;
	vec->capacity	   = init_capacity;
	vec->el_size	   = el_size;
	vec->length	   = 0;
	return vec;
}

void vector_free(struct vector *vec)
{
	if (!vec) {
		return;
	}
	free(vec->data);
	free(vec);
}

size_t vector_get_len(const struct vector *vec) { return vec->length; }
inline void *vector_get(struct vector *vec, size_t index)
{
	if (index < vec->length) {
		return (char *)vec->data + index * vec->el_size;
	}
	return NULL;
}

inline static int vector_realloc(struct vector *vec, size_t new_capacity)
{
	void *new_data = realloc(vec->data, new_capacity * vec->el_size);

	if (new_data == NULL)
		return 1;

	vec->data = new_data;
	if (new_capacity > vec->capacity) {
		size_t new_items_n     = new_capacity - vec->capacity;
		char *old_cap_boundary = (char *)vec->data + new_items_n * vec->el_size;
		memset(old_cap_boundary, 0, // NOLINT
		       (new_capacity - new_items_n) * vec->el_size);
	}

	vec->capacity = new_capacity;
	return 0;
}

int vector_add(struct vector *vec, void *element)
{
	if (vec->length == vec->capacity) {
		int new_capacity = vec->capacity * 2;
		if (vector_realloc(vec, new_capacity) > 0) {
			return 1;
		}
	}

	char *data = (char *)vec->data;
	void *dest = data + (vec->length * vec->el_size);
	memcpy(dest, element, vec->el_size); // NOLINT
	++vec->length;

	return 0;
}

ssize_t vector_find(struct vector *vec, void *filter_entry, bool(filter_func)(void *entry, void *filter_entry))
{
	for (size_t i = 0; i < vector_get_len(vec); ++i) {
		void *entry = vector_get(vec, i);

		if (filter_func(entry, filter_entry)) {
			return i;
		}
	}
	return -1;
}

int vector_del(struct vector *vec, size_t idx)
{
	if (idx >= vec->length) {
		return 1;
	}

	char *data	   = (char *)vec->data;
	void *el_addr	   = data + (idx * vec->el_size);
	void *next_el_addr = data + ((idx + 1) * vec->el_size);

	size_t n = (vec->length - idx) * vec->el_size;
	memmove(el_addr, next_el_addr, n); // NOLINT
	--vec->length;

	// make sure there are at least init_capacity elements
	if (vec->length > vec->init_capacity) {
		size_t new_capacity = vec->capacity / 2;
		if (new_capacity > 0 && vec->length < new_capacity) {
			// assume no failure as we decrease the size
			vector_realloc(vec, new_capacity);
		}
	}

	return 0;
}
