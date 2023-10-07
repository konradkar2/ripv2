#include "vector.h"

void vector_init(vector *vec, size_t total_length, void *data, size_t el_size)
{
	assert("invalid vector data" && total_length > 0);
	assert("invalid vector data" && el_size > 0);

	vec->data	  = data;
	vec->total_length = total_length;
	vec->el_size	  = el_size;
	vec->length	  = 0;
}

inline size_t vec_get_len_total(vector *vec) { return vec->total_length; }
inline size_t vec_get_len_current(vector *vec) { return vec->length; }
inline size_t vec_get_len_left(vector *vec)
{
	return vec->total_length - vec->length;
}

inline void *vector_get_el(vector *vec, size_t index)
{
	return (char *)vec->data + index * vec->el_size;
}

int vector_add_el(vector *vec, void *element, size_t el_size)
{
	assert(vec->el_size == el_size);
	if (vec_get_len_left(vec) == 0) {
		return 1;
	}

	char *data = (char *)vec->data;
	void *dest = data + (vec->length * vec->el_size);
	memcpy(dest, element, el_size);
	++vec->length;

	return 0;
}

int vector_del_el(vector *vec, size_t idx)
{
	if (idx >= vec->length) {
		return 1;
	}

	char *data = (char *)vec->data;
	void *dest = data + (idx * vec->el_size);
	void *src  = data + ((idx + 1) * vec->el_size);
	size_t n   = (vec->total_length - idx) * vec->el_size;
	memmove(dest, src, n);

	return 0;
}