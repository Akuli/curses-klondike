#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "card.h"
#include "misc.h"
#include "sol.h"


static bool initscred = false;

// https://stackoverflow.com/a/8562768
static void exitcb(void)
{
	if (initscred) {
		endwin();
		initscred = false;
	}
}
void (*onerrorexit)(void) = exitcb;


int main(void)
{
	// displaying unicodes correctly needs setlocale here AND -lncursesw in makefile
	if (!setlocale(LC_ALL, ""))
		fatal_error("setlocale() failed");

	time_t t = time(NULL);
	if (t == (time_t)(-1))
		fatal_error("time() failed");
	srand(t);

	struct Card *list = card_createallshuf();

	struct Sol sol, sol2;
	sol_init(&sol, list);
	sol_dup(sol, &sol2);
	sol_debug(sol2);
	sol_free(sol);
	sol_free(sol2);

	/*
	initscr();
	initscred = true;

	printw("asd asd \xe2\x99\xa0");
	refresh();
	getch();
	endwin();
	*/

	return 0;
}
