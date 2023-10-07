#include "test.h"
#include "vector.h"

#define ARRAY_LEN 100

typedef struct {
	char a;
	int b;
} example_data;

REGISTER_TEST(vector_add_and_len_test)
{
	example_data array[ARRAY_LEN] = {};
	vector vec;
	vector *vecp = &vec;
	vector_init(vecp, ARRAY_LEN, array, sizeof(example_data));

	ASSERT(vec_get_len_current(vecp) == 0);
	ASSERT(vec_get_len_left(vecp) == ARRAY_LEN);
	ASSERT(vec_get_len_total(vecp) == ARRAY_LEN);

	for (size_t i = 0; i < ARRAY_LEN; ++i) {
		example_data data = {.a = 'c', .b = i};
		int res = vector_add_el(vecp, &data, sizeof(example_data));
		ASSERT(res == 0);

		size_t expected_len = i + 1;
		ASSERT(vec_get_len_current(vecp) == expected_len);
		ASSERT(vec_get_len_left(vecp) == ARRAY_LEN - expected_len);
		ASSERT(vec_get_len_total(vecp) == ARRAY_LEN);
	}
	example_data data = {};
	int res		  = vector_add_el(vecp, &data, sizeof(example_data));
	ASSERT(res != 0);
}

REGISTER_TEST(vector_add_and_get)
{
	example_data array[ARRAY_LEN] = {};
	vector vec;
	vector *vecp = &vec;
	vector_init(vecp, ARRAY_LEN, array, sizeof(example_data));

	example_data data = {.a = 'c', .b = 5};
	vector_add_el(vecp, &data, sizeof(example_data));

	data.a = 'b';
	data.b = 6;
	vector_add_el(vecp, &data, sizeof(example_data));

	ASSERT(vec_get_len_current(vecp) == 2);

	{
		example_data *data0 = vector_get_el(vecp, 0);
		ASSERT(data0 && data0->a == 'c' && data0->b == 5);
	}

	{

		example_data *data1 = vector_get_el(vecp, 1);
		ASSERT(data1 && data1->a == 'b' && data1->b == 6);
	}
}