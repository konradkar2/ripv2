#include "test.h"
#include "utils.h"
#include <utils/vector.h>
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
	ASSERT(vector_get_len(vec) == 0);

	for (int i = 0; i < VEC_SIZE; ++i) {
		example_data data = {.a = 'c', .b = i};
		int res = vector_add(vec, &data, sizeof(example_data));
		ASSERT(res == 0);

		size_t expected_len = i + 1;
		ASSERT(vector_get_len(vec) == expected_len);
	}

	for (int i = 0; i < VEC_SIZE; ++i) {
		example_data *data = vector_get(vec, i);
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
	vector_add(vec, &data, sizeof(example_data));

	data.a = 'b';
	data.b = 6;
	vector_add(vec, &data, sizeof(example_data));

	ASSERT(vector_get_len(vec) == 2);

	{
		example_data *data0 = vector_get(vec, 0);
		ASSERT(data0 && data0->a == 'c' && data0->b == 5);
	}

	{
		example_data *data1 = vector_get(vec, 1);
		ASSERT(data1 && data1->a == 'b' && data1->b == 6);
	}

	example_data *data2 = vector_get(vec, 2);
	ASSERT(data2 == NULL);

	vector_free(vec);
}

REGISTER_TEST(vector_del)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 2, sizeof(example_data));
	ASSERT(vec);
	ASSERT(1 == vector_del(vec, 0));

	example_data data0 = {.a = 'c', .b = 5};
	vector_add(vec, &data0, sizeof(example_data));
	example_data data1 = {.a = 'd', .b = 6};
	vector_add(vec, &data1, sizeof(example_data));
	example_data data2 = {.a = 'e', .b = 7};
	vector_add(vec, &data2, sizeof(example_data));

	ASSERT(0 == vector_del(vec, 1));
	ASSERT(2 == vector_get_len(vec));

	example_data *data = vector_get(vec, 0);
	ASSERT(data && data->a == 'c' && data->b == 5);

	data = vector_get(vec, 1);
	ASSERT(data && data->a == 'e' && data->b == 7);

	ASSERT(NULL == vector_get(vec, 2));

	vector_free(vec);
}

REGISTER_TEST(vector_realloc)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 10, sizeof(example_data));
	ASSERT(vec);
	ASSERT(vector_get_len(vec) == 0);

	for (size_t i = 0; i < VEC_SIZE; ++i) {
		example_data data = {.a = 'c', .b = i};
		int res = vector_add(vec, &data, sizeof(example_data));
		ASSERT(res == 0);

		size_t expected_len = i + 1;
		ASSERT(vector_get_len(vec) == expected_len);
	}

	for (int idx = VEC_SIZE - 1; idx >= 0; --idx) {
		int res = vector_del(vec, idx);
		ASSERT(res == 0);

		size_t expected_len = idx;
		ASSERT(vector_get_len(vec) == expected_len);
	}
	int res = vector_del(vec, 0);
	assert(res == 1);

	vector_free(vec);
}

bool example_data_cmp(void *entry, void *filter_entry)
{
	example_data *e1 = entry;
	example_data *e2 = filter_entry;
	return e1->a == e2->a && e1->b == e2->b;
}

REGISTER_TEST(vector_get_by_filter)
{
	struct vector *vec = NULL;
	vec		   = vector_create(VEC_SIZE / 2, sizeof(example_data));
	ASSERT(vec);

	example_data data = {.a = 'c', .b = 5};
	vector_add(vec, &data, sizeof(example_data));

	data.a = 'b';
	data.b = 6;
	vector_add(vec, &data, sizeof(example_data));

	data.a = 'd';
	data.b = 100;
	vector_add(vec, &data, sizeof(example_data));

	example_data data_filter = {.a = 'b', .b = 6};
	ssize_t found_idx =
	    vector_find(vec, &data_filter, example_data_cmp);
	ASSERT(found_idx != -1);
	example_data * found = vector_get(vec, found_idx);
	ASSERT(found->a == 'b' && found->b == 6);

	vector_free(vec);
}
