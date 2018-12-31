#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "card.h"
#include "misc.h"
#include "sol.h"
#include "ui.h"


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
	// displaying unicodes correctly needs setlocale here AND cursesw instead of curses in makefile
	if (!setlocale(LC_ALL, ""))
		fatal_error("setlocale() failed");

	time_t t = time(NULL);
	if (t == (time_t)(-1))
		fatal_error("time() failed");
	srand(t);

	if (!initscr())
		fatal_error("initscr() failed");
	initscred = true;

	struct Card *list = card_createallshuf();

	struct Sol sol;
	sol_init(&sol, list);
	for (int i=0; i < 100; i++) {
		sol_stock2discard(&sol);
		//sol_debug(sol);
	}

	refresh();   // yes, this is needed before drawing the cards

	ui_drawcard(stdscr, *sol.tableau[0], 0, 0, 0, 0);
	ui_drawcard(stdscr, *sol.tableau[1], 0, 0, 1, 0);
	ui_drawcard(stdscr, *sol.tableau[2], 0, 0, 2, 0);
	ui_drawcard(stdscr, *sol.tableau[0], 0, 0, 3, 0);
	ui_drawcard(stdscr, *sol.tableau[3]->next, 6, 1, 0, 0);
	ui_drawcard(stdscr, *sol.tableau[3]->next->next, 6, 1, 0, 1);
	getch();
	sol_free(sol);
	endwin();

	return 0;
}
