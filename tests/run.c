#include <stdio.h>

static void donothing(void) { }

// externed in src/misc.h
void (*onerrorexit)(void) = donothing;


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
	RUN_TEST(card_suit_all);
	RUN_TEST(card_suit_color);
	RUN_TEST(card_createallshuf_free);
	RUN_TEST(card_createallshuf_gives_all_52_cards);
	RUN_TEST(card_str);
	RUN_TEST(card_top);
	RUN_TEST(card_popbot);
	RUN_TEST(card_pushtop);
	RUN_TEST(card_inlist);
	return 0;
}
