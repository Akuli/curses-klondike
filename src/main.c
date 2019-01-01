#include <assert.h>
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
// onerrorexit is externed in misc.h
static void exitcb(void)
{
	if (initscred) {
		endwin();
		initscred = false;
	}
}
void (*onerrorexit)(void) = exitcb;


struct Game {
	struct UiSelection sel;
	struct Sol sol;
};

// returns whether to continue playing
static bool handle_key(struct Game *gam, int k)
{
	SolCardPlace plcs[2+4+7];
	SolCardPlace *ptr = plcs;
	*ptr++ = SOL_STOCK;
	*ptr++ = SOL_DISCARD;
	for (int i=0; i < 4; i++)
		*ptr++ = SOL_FOUNDATION(i);
	for (int i=0; i < 7; i++)
		*ptr++ = SOL_TABLEAU(i);

	// TODO: h help
	switch(k) {
	case 'q':
		return false;

	case 'd':
		sol_stock2discard(&gam->sol);
		break;

	case KEY_LEFT:
	case KEY_RIGHT:
		assert(1);   // does nothing, needed here because c syntax
		int i;
		for (i=0; ; i++) {
			assert((unsigned)i < sizeof(plcs)/sizeof(plcs[0]));
			if (plcs[i] == gam->sel.place)
				break;
		}

		i += (k == KEY_LEFT) ? -1 : 1;

		// TODO: use %, careful with signs
		int n = sizeof(plcs)/sizeof(plcs[0]);
		if (i < 0)
			i += n;
		if (i >= n)
			i -= n;
		gam->sel.place = plcs[i];
		break;

	default:
		break;
	}

	return true;
}

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

	if (cbreak() == ERR)
		fatal_error("cbreak() failed");
	if (curs_set(0) == ERR)
		fatal_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR)
		fatal_error("keypad() failed");

	refresh();   // yes, this is needed before drawing the cards for some reason

	struct Game gam;
	sol_init(&gam.sol, card_createallshuf());
	gam.sel.card = NULL;
	gam.sel.place = SOL_STOCK;

	sol_move(&gam.sol, card_top(gam.sol.tableau[5]), SOL_FOUNDATION(0));
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(0));
	sol_move(&gam.sol, card_top(gam.sol.tableau[3]), SOL_TABLEAU(0));
	sol_move(&gam.sol, gam.sol.tableau[0], SOL_TABLEAU(3));
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(3));
	sol_move(&gam.sol, card_top(gam.sol.tableau[5]), SOL_TABLEAU(3));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(4));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(0));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(5));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(0));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(1));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(1));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(2));
	sol_move(&gam.sol, card_top(gam.sol.tableau[4]), SOL_FOUNDATION(2));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(0));
	sol_move(&gam.sol, card_top(gam.sol.tableau[6]), SOL_TABLEAU(0));
	sol_move(&gam.sol, gam.sol.tableau[5]->next->next->next, SOL_TABLEAU(6));
	sol_move(&gam.sol, card_top(gam.sol.tableau[5]), SOL_TABLEAU(0));
	sol_move(&gam.sol, card_top(gam.sol.tableau[5]), SOL_TABLEAU(3));
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(0));
	sol_move(&gam.sol, card_top(gam.sol.tableau[1]), SOL_TABLEAU(0));
	sol_move(&gam.sol, card_top(gam.sol.tableau[5]), SOL_TABLEAU(1));  // makes room for a K
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(1));
	sol_move(&gam.sol, card_top(gam.sol.tableau[4]), SOL_TABLEAU(1));
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(2));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(0));
	sol_stock2discard(&gam.sol);
	sol_stock2discard(&gam.sol);
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(5));
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(5));
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_TABLEAU(5));
	sol_move(&gam.sol, gam.sol.tableau[3]->next->next, SOL_TABLEAU(5));
	sol_move(&gam.sol, card_top(gam.sol.tableau[3]), SOL_FOUNDATION(1));
	sol_move(&gam.sol, card_top(gam.sol.tableau[3]), SOL_FOUNDATION(0));
	sol_move(&gam.sol, card_top(gam.sol.discard), SOL_FOUNDATION(0));

	do {
		ui_drawsol(stdscr, gam.sol, gam.sel);
		refresh();
	} while( handle_key(&gam, getch()) );

	sol_free(gam.sol);
	endwin();

	return 0;
}
