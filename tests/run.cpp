#include <cstdio>
#include <cstdlib>
#include <ctime>

static void run_test(const char *nam, void (*f)())
{
	std::printf("%-50s  ", nam);
	std::fflush(stdout);
	f();
	std::printf("ok\n");
}

int main()
{
	unsigned int s = time(nullptr);
	std::printf("srand seed: %u\n", s);
	srand(s);

	#define RUN_TEST(f) do { void f(); run_test(#f, f); } while(0)

	RUN_TEST(test_card_numstring);
	RUN_TEST(test_cardlist_init);
	RUN_TEST(test_cardlist_top);
	RUN_TEST(test_cardlist_pop);
	RUN_TEST(test_cardlist_push);

	RUN_TEST(test_klon_init);
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
