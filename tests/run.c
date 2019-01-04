#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <src/help.h>

static void donothing(void) { }

// externed in src/misc.h
void (*onerrorexit)(void) = donothing;

// externed in help.h
struct Help help[] = { {NULL, NULL} };


static void run_test(char *nam, void (*f)(void))
{
	printf("%-50s  ", nam);
	fflush(stdout);
	f();
	printf("ok\n");
}

#define RUN_TEST(f) do { void test_##f(void); run_test("test_" #f, test_##f); } while(0)

int main(void)
{
	unsigned int s = time(NULL);
	printf("srand seed: %u\n", s);
	srand(s);

	RUN_TEST(card_suit_all);
	RUN_TEST(card_suit_color);
	RUN_TEST(card_createallshuf_free);
	RUN_TEST(card_createallshuf_gives_all_52_cards);
	RUN_TEST(card_str);
	RUN_TEST(card_top);
	RUN_TEST(card_popbot);
	RUN_TEST(card_pushtop);
	RUN_TEST(card_inlist);

	RUN_TEST(sol_cardplace);
	RUN_TEST(sol_init_free);
	RUN_TEST(sol_dup);
	RUN_TEST(sol_canmove);
	RUN_TEST(sol_move);
	RUN_TEST(sol_stock2discard);
	return 0;
}
