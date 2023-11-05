#include "test.h"
#include "utils.h"
#include "vector.h"
#include <assert.h>

typedef struct {
	char a;
	int b;
} example_data;

#define VEC_SIZE 1001

REGISTER_TEST(vector_add_and_len_test)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 2, sizeof(example_data));
	ASSERT(vec);
	ASSERT(vec_get_len(vec) == 0);

	for (int i = 0; i < VEC_SIZE; ++i) {
		example_data data = {.a = 'c', .b = i};
		int res = vector_add_el(vec, &data, sizeof(example_data));
		ASSERT(res == 0);

		size_t expected_len = i + 1;
		ASSERT(vec_get_len(vec) == expected_len);
	}

	for (int i = 0; i < VEC_SIZE; ++i) {
		example_data * data = vector_get_el(vec, i);
		ASSERT(data);
		ASSERT(data && data->a == 'c' && data->b == i);
	}

	vector_free(vec);
}

REGISTER_TEST(vector_add)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 2, sizeof(example_data));
	ASSERT(vec);

	example_data data = {.a = 'c', .b = 5};
	vector_add_el(vec, &data, sizeof(example_data));

	data.a = 'b';
	data.b = 6;
	vector_add_el(vec, &data, sizeof(example_data));

	ASSERT(vec_get_len(vec) == 2);

	{
		example_data *data0 = vector_get_el(vec, 0);
		ASSERT(data0 && data0->a == 'c' && data0->b == 5);
	}

	{
		example_data *data1 = vector_get_el(vec, 1);
		ASSERT(data1 && data1->a == 'b' && data1->b == 6);
	}

	example_data *data2 = vector_get_el(vec, 2);
	ASSERT(data2 == NULL);

	vector_free(vec);
}

REGISTER_TEST(vector_del)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 2, sizeof(example_data));
	ASSERT(vec);
	ASSERT(1 == vector_del_el(vec, 0));

	example_data data0 = {.a = 'c', .b = 5};
	vector_add_el(vec, &data0, sizeof(example_data));
	example_data data1 = {.a = 'd', .b = 6};
	vector_add_el(vec, &data1, sizeof(example_data));
	example_data data2 = {.a = 'e', .b = 7};
	vector_add_el(vec, &data2, sizeof(example_data));

	ASSERT(0 == vector_del_el(vec, 1));
	ASSERT(2 == vec_get_len(vec));

	example_data *data = vector_get_el(vec, 0);
	ASSERT(data && data->a == 'c' && data->b == 5);

	data = vector_get_el(vec, 1);
	ASSERT(data && data->a == 'e' && data->b == 7);

	ASSERT(NULL == vector_get_el(vec, 2));

	vector_free(vec);
}

REGISTER_TEST(vector_realloc)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 10, sizeof(example_data));
	ASSERT(vec);
	ASSERT(vec_get_len(vec) == 0);

	for (size_t i = 0; i < VEC_SIZE; ++i) {
		example_data data = {.a = 'c', .b = i};
		int res = vector_add_el(vec, &data, sizeof(example_data));
		ASSERT(res == 0);

		size_t expected_len = i + 1;
		ASSERT(vec_get_len(vec) == expected_len);
	}

	for (int idx = VEC_SIZE - 1; idx >= 0; --idx) {
		int res = vector_del_el(vec, idx);
		ASSERT(res == 0);

		size_t expected_len = idx;
		ASSERT(vec_get_len(vec) == expected_len);
	}
	int res = vector_del_el(vec, 0);
	assert(res == 1);

	vector_free(vec);
}
