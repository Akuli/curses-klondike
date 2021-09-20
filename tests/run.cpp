#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/help.hpp"

static void run_test(const char *nam, void (*f)())
{
	std::printf("%-50s  ", nam);
	std::fflush(stdout);
	f();
	std::printf("ok\n");
}

#define RUN_TEST(f) do { void f(); run_test(#f, f); } while(0)

void init_args_tests();
void deinit_args_tests();

int main()
{
	unsigned int s = time(nullptr);
	std::printf("srand seed: %u\n", s);
	srand(s);

	RUN_TEST(test_card_createallshuf_gives_all_52_cards);
	RUN_TEST(test_card_str);
	RUN_TEST(test_card_top);
	RUN_TEST(test_card_popbot);
	RUN_TEST(test_card_pushtop);

	RUN_TEST(test_klon_init_free);
	RUN_TEST(test_klon_dup);
	RUN_TEST(test_klon_canmove);
	RUN_TEST(test_klon_move);
	RUN_TEST(test_klon_stock2discard);

	RUN_TEST(test_args_help);
	RUN_TEST(test_args_defaults);
	RUN_TEST(test_args_no_defaults);
	RUN_TEST(test_args_errors);
	RUN_TEST(test_args_nused_bug);

	return 0;
}
