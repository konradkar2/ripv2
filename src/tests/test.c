#include "test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_TESTS 1024
test_fixture all_tests[MAX_TESTS];
size_t num_tests = 0;

int run_all_tests(void)
{
	int ret	      = 0;
	size_t failed = 0;
	for (size_t i = 0; i < num_tests; ++i) {
		test_fixture *test = &all_tests[i];

		int status = 0;
		test->func(&status);
		if (status) {
			printf("FAILED %s \n", test->name);
			++failed;
			ret = 1;
		} else {
			printf("PASSED %s \n", test->name);
		}
	}

	if (failed) {
		printf("%zu out of %zu tests FAILED\n", failed, num_tests);
	} else {
		printf("All of %zu tests PASSED\n", num_tests);
	}

	return ret;
}

void add_test(test_fixture test)
{
	assert("numtest" && num_tests < MAX_TESTS);

	all_tests[num_tests] = test;
	num_tests++;
}

int str_eq_n(char *expected, char *actual, size_t len)
{
	if (strncmp(expected, actual, len) == 0) {
		return 0;
	}
	printf("expected: %s, actual %s \n", expected, actual);
	return 1;
}

int str_eq(char *expected, char *actual)
{
	if (strcmp(expected, actual) == 0) {
		return 0;
	}
	printf("expected: %s, actual %s \n", expected, actual);
	return 1;
}
