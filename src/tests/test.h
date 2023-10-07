#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>

typedef struct {
	const char *name;
	void (*func)(int *status);
	int status;
} test_fixture;

int run_all_tests();
void add_test(test_fixture test);

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
	void TEST_##TEST_NAME(int *_test_status);                              \
	__attribute__((constructor)) void register_##TEST_NAME()               \
	{                                                                      \
		test_fixture test;                                             \
		test.name = #TEST_NAME;                                        \
		test.func = TEST_##TEST_NAME;                                  \
		add_test(test);                                                \
	}                                                                      \
	void TEST_##TEST_NAME(__attribute__((unused)) int *_test_status)

#endif