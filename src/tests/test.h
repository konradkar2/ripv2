#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>

typedef struct {
	const char *name;
	void (*func)(int *status);
} test_fixture;

int run_all_tests(void);
void add_test(test_fixture test);

int str_eq(char *expected, char *actual);
#define STR_EQ(expected, actual) ASSERT(0 == str_eq(expected, actual))
int str_eq_n(char *expected, char *actual, size_t len);
#define STR_EQ_N(expected, actual, len) ASSERT(0 == str_eq_n(expected, actual, len))

#define ASSERT(expr)                                                                               \
	do {                                                                                       \
		if (expr) {                                                                        \
			; /* empty */                                                              \
		} else {                                                                           \
			printf("assert: " #expr " failed %s:%d\n", __FILE__, __LINE__);            \
			*test_status = 1;                                                          \
			return;                                                                    \
		}                                                                                  \
	} while (0)

#define REGISTER_TEST(TEST_NAME)                                                                   \
	void TEST_##TEST_NAME(int *_test_status);                                                  \
	__attribute__((constructor)) void register_##TEST_NAME(void)                               \
	{                                                                                          \
		test_fixture test;                                                                 \
		test.name = #TEST_NAME;                                                            \
		test.func = TEST_##TEST_NAME;                                                      \
		add_test(test);                                                                    \
	}                                                                                          \
	void TEST_##TEST_NAME(__attribute__((unused)) int *test_status)

#endif
