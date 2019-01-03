#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "card.h"
#include "misc.h"
#include "sel.h"
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


// mv2 = move to
struct Game {
	struct Sol sol;
	struct UiSelection sel;  // the card moving or about to be moved
	SolCardPlace mv2;        // where to move the card, or 0 if user is not moving
};

static enum SelDirection curses_key_to_seldirection(int k)
{
	switch(k) {
	case KEY_LEFT: return SEL_LEFT;
	case KEY_RIGHT: return SEL_RIGHT;
	case KEY_UP: return SEL_UP;
	case KEY_DOWN: return SEL_DOWN;
	default: assert(0);
	}
}

// returns whether to continue playing
static bool handle_key(struct Game *gam, int k)
{
	// TODO: h help
	// TODO: esc quit, in addition to q
	// TODO: f foundationing
	switch(k) {
	case 'q':
		return false;

	case 'd':
		if (!gam->mv2)
			sol_stock2discard(&gam->sol);
		break;

	case KEY_LEFT:
	case KEY_RIGHT:
	case KEY_UP:
	case KEY_DOWN:
		if (gam->mv2)
			sel_anothercardmv(gam->sol, gam->sel, curses_key_to_seldirection(k), &gam->mv2);
		else
			sel_anothercard(gam->sol, &gam->sel, curses_key_to_seldirection(k));
		break;

	case '\n':
		if (gam->mv2) {
			sel_endmv(&gam->sol, &gam->sel, gam->mv2);
			gam->mv2 = 0;
		}
		else if (gam->sel.place == SOL_STOCK)
			sol_stock2discard(&gam->sol);
		else if (gam->sel.card && gam->sel.card->visible)
			gam->mv2 = gam->sel.place;
		break;

	default:
		break;
	}

	return true;
}

// creates a temp copy of the sol, modifies it nicely and calls ui_drawsol
static void draw_sol_with_mv(WINDOW *win, struct Sol sol, struct UiSelection sel, SolCardPlace mv)
{
	struct Sol tmpsol;
	struct UiSelection tmpsel = { .place = mv };

	tmpsel.card = sol_dup(sol, &tmpsol, sel.card);
	sol_rawmove(&tmpsol, tmpsel.card, tmpsel.place);

	ui_drawsol(win, tmpsol, tmpsel);
	sol_free(tmpsol);
}

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
	gam.mv2 = 0;

	do {
		if (gam.mv2)
			draw_sol_with_mv(stdscr, gam.sol, gam.sel, gam.mv2);
		else
			ui_drawsol(stdscr, gam.sol, gam.sel);
		refresh();
	} while( handle_key(&gam, getch()) );

	sol_free(gam.sol);
	endwin();

	return 0;
}
