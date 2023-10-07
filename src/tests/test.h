#ifndef TEST_H
#define TEST_H

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	const char *name;
	void (*func)(int *status);
	int status;
} test_fixture;

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

#define ASSERT(expr)                                                           \
	do {                                                                   \
		if (expr) {                                                    \
			; /* empty */                                          \
		} else {                                                       \
			printf("assert: " #expr " failed %s:%d\n", __FILE__,   \
			       __LINE__);                                      \
			*_test_status = 1;                                     \
			return;                                                \
		}                                                              \
	} while (0)

#define REGISTER_TEST(TEST_NAME)                                               \
	void TEST_NAME(int *_test_status);                                     \
	__attribute__((constructor)) void register_##TEST_NAME()               \
	{                                                                      \
		assert("max number of tests reached" &&                        \
		       num_tests < MAX_TESTS);                                 \
		all_tests[num_tests].name = #TEST_NAME;                        \
		all_tests[num_tests].func = TEST_NAME;                         \
		num_tests++;                                                   \
	}                                                                      \
	void TEST_NAME(__attribute__((unused)) int *_test_status)

#endif