#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void run_test(const char *name, void (*f)())
{
    printf("%-50s  ", name);
    fflush(stdout);
    f();
    printf("ok\n");
}

int main()
{
    unsigned int s = time(NULL);
    printf("srand seed: %u\n", s);
    srand(s);

    #define RUN_TEST(f) do { void f(); run_test(#f, f); } while(0)

    /*
    RUN_TEST(test_card_numstring);
    RUN_TEST(test_cardlist_init);
    RUN_TEST(test_cardlist_top);
    RUN_TEST(test_cardlist_pop);
    RUN_TEST(test_cardlist_push);

    RUN_TEST(test_klondike_init);
    RUN_TEST(test_klondike_dup);
    RUN_TEST(test_klondike_canmove);
    RUN_TEST(test_klondike_move);
    RUN_TEST(test_klondike_stock2discard);
    */

    RUN_TEST(test_args_help);
    RUN_TEST(test_args_defaults);
    RUN_TEST(test_args_no_defaults);
    RUN_TEST(test_args_errors);
    RUN_TEST(test_args_nused_bug);

    return 0;
}
