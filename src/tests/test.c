#include "test.h"
#include <assert.h>

#define MAX_TESTS 1024
test_fixture all_tests[MAX_TESTS];
int num_tests = 0;

int run_all_tests()
{
	int ret = 0;
	for (int i = 0; i < num_tests; ++i) {
		test_fixture *test = &all_tests[i];

		int status = 0;
		test->func(&status);
		if (status) {
			printf("%s FAILED\n", test->name);
			ret = 1;
		} else {
			printf("%s PASSED\n", test->name);
		}
	}
	return ret;
}

void add_test(test_fixture test)
{
	assert("numtest" && num_tests < MAX_TESTS);

	all_tests[num_tests] = test;
	num_tests++;
}