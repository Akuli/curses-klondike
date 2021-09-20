#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/help.hpp"

static void run_test(const char *nam, void (*f)(void))
{
	std::printf("%-50s  ", nam);
	fflush(stdout);
	f();
	std::printf("ok\n");
}

#define RUN_TEST(f) do { void test_##f(void); run_test("test_" #f, test_##f); } while(0)

void init_args_tests(void);
void deinit_args_tests(void);

int main(void)
{
	unsigned int s = time(nullptr);
	std::printf("srand seed: %u\n", s);
	srand(s);

	RUN_TEST(card_createallshuf_gives_all_52_cards);
	RUN_TEST(card_str);
	RUN_TEST(card_top);
	RUN_TEST(card_popbot);
	RUN_TEST(card_pushtop);

	RUN_TEST(klon_init_free);
	RUN_TEST(klon_dup);
	RUN_TEST(klon_canmove);
	RUN_TEST(klon_move);
	RUN_TEST(klon_stock2discard);

	RUN_TEST(args_help);
	RUN_TEST(args_defaults);
	RUN_TEST(args_no_defaults);
	RUN_TEST(args_errors);
	RUN_TEST(args_nused_bug);

	return 0;
}
