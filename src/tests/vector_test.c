#include "test.h"
#include "vector.h"

#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))

typedef struct {
	char a;
	int b;
} example_data;

REGISTER_TEST(vector_add_and_len_test)
{
	example_data array[100] = {0};
	vector vec;
	vector *vecp = &vec;
	vector_init(vecp, ARRAY_LEN(array), array, sizeof(example_data));

	ASSERT(vec_get_len(vecp) == 0);
	ASSERT(vec_get_len_left(vecp) == ARRAY_LEN(array));
	ASSERT(vec_get_len_total(vecp) == ARRAY_LEN(array));

	for (size_t i = 0; i < ARRAY_LEN(array); ++i) {
		example_data data = {.a = 'c', .b = i};
		int res = vector_add_el(vecp, &data, sizeof(example_data));
		ASSERT(res == 0);

		size_t expected_len = i + 1;
		ASSERT(vec_get_len(vecp) == expected_len);
		ASSERT(vec_get_len_left(vecp) ==
		       ARRAY_LEN(array) - expected_len);
		ASSERT(vec_get_len_total(vecp) == ARRAY_LEN(array));
	}
	example_data data = {0};
	int res		  = vector_add_el(vecp, &data, sizeof(example_data));
	ASSERT(res != 0);
}

REGISTER_TEST(vector_add)
{
	example_data array[100] = {0};
	vector vec;
	vector *vecp = &vec;
	vector_init(vecp, ARRAY_LEN(array), array, sizeof(example_data));

	example_data data = {.a = 'c', .b = 5};
	vector_add_el(vecp, &data, sizeof(example_data));

	data.a = 'b';
	data.b = 6;
	vector_add_el(vecp, &data, sizeof(example_data));

	ASSERT(vec_get_len(vecp) == 2);

	{
		example_data *data0 = vector_get_el(vecp, 0);
		ASSERT(data0 && data0->a == 'c' && data0->b == 5);
	}

	{
		example_data *data1 = vector_get_el(vecp, 1);
		ASSERT(data1 && data1->a == 'b' && data1->b == 6);
	}

	example_data *data2 = vector_get_el(vecp, 2);
	ASSERT(data2 == NULL);
}

REGISTER_TEST(vector_del)
{
	example_data array[3] = {0};
	vector vec;
	vector *vecp = &vec;
	vector_init(vecp, ARRAY_LEN(array), array, sizeof(example_data));
	ASSERT(0 != vector_del_el(vecp, 0));

	example_data data0 = {.a = 'c', .b = 5};
	vector_add_el(vecp, &data0, sizeof(example_data));
	example_data data1 = {.a = 'd', .b = 6};
	vector_add_el(vecp, &data1, sizeof(example_data));
	example_data data2 = {.a = 'e', .b = 7};
	vector_add_el(vecp, &data2, sizeof(example_data));

	ASSERT(0 == vector_del_el(vecp, 1));
	ASSERT(2 == vec_get_len(vecp));

	example_data *data = vector_get_el(vecp, 0);
	ASSERT(data && data->a == 'c' && data->b == 5);

	data = vector_get_el(vecp, 1);
	ASSERT(data && data->a == 'e' && data->b == 7);

	ASSERT(NULL == vector_get_el(vecp, 2));
}
