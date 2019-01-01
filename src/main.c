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
	//srand(t);

	if (!initscr())
		fatal_error("initscr() failed");
	initscred = true;

	struct Card *list = card_createallshuf();

	struct Sol sol;
	sol_init(&sol, list);

	refresh();   // yes, this is needed before drawing the cards

	sol_move(&sol, card_top(sol.tableau[5]), SOL_FOUNDATION(0));
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(0));
	sol_move(&sol, card_top(sol.tableau[3]), SOL_TABLEAU(0));
	sol_move(&sol, sol.tableau[0], SOL_TABLEAU(3));
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(3));
	sol_move(&sol, card_top(sol.tableau[5]), SOL_TABLEAU(3));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(4));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(0));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(5));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(0));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(1));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(1));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(2));
	sol_move(&sol, card_top(sol.tableau[4]), SOL_FOUNDATION(2));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(0));
	sol_move(&sol, card_top(sol.tableau[6]), SOL_TABLEAU(0));
	sol_move(&sol, sol.tableau[5]->next->next->next, SOL_TABLEAU(6));
	sol_move(&sol, card_top(sol.tableau[5]), SOL_TABLEAU(0));
	sol_move(&sol, card_top(sol.tableau[5]), SOL_TABLEAU(3));
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(0));
	sol_move(&sol, card_top(sol.tableau[1]), SOL_TABLEAU(0));
	sol_move(&sol, card_top(sol.tableau[5]), SOL_TABLEAU(1));  // makes room for a K
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(1));
	sol_move(&sol, card_top(sol.tableau[4]), SOL_TABLEAU(1));
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(2));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(0));
	sol_stock2discard(&sol);
	sol_stock2discard(&sol);
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(5));
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(5));
	sol_move(&sol, card_top(sol.discard), SOL_TABLEAU(5));
	sol_move(&sol, sol.tableau[3]->next->next, SOL_TABLEAU(5));
	sol_move(&sol, card_top(sol.tableau[3]), SOL_FOUNDATION(1));
	sol_move(&sol, card_top(sol.tableau[3]), SOL_FOUNDATION(0));
	sol_move(&sol, card_top(sol.discard), SOL_FOUNDATION(0));

	while (true) {
		ui_drawsol(stdscr, sol, (struct UiSelection){ .card = sol.tableau[5]->next->next->next, .place = SOL_TABLEAU(5) });
		refresh();

		int c = getch();
		if (c == 'q')
			break;
		if (c == 'd')
			sol_stock2discard(&sol);
	}

	sol_free(sol);
	endwin();

	return 0;
}
