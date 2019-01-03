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
static bool handle_key(struct Sol *sol, struct UiSelection *sel, SolCardPlace *mv, int k)
{
	// TODO: h help
	// TODO: esc quit, in addition to q OR esc for unselecting
	// TODO: f foundationing
	switch(k) {
	case 'q':
		return false;

	case 'd':
		if (!*mv)
			sol_stock2discard(sol);
		break;

	case KEY_LEFT:
	case KEY_RIGHT:
	case KEY_UP:
	case KEY_DOWN:
		if (*mv)
			sel_anothercardmv(*sol, *sel, curses_key_to_seldirection(k), mv);
		else
			sel_anothercard(*sol, sel, curses_key_to_seldirection(k));
		break;

	case '\n':
		if (*mv) {
			sel_endmv(sol, sel, *mv);
			*mv = 0;
		}
		else if (sel->place == SOL_STOCK)
			sol_stock2discard(sol);
		else if (sel->card && sel->card->visible)
			*mv = sel->place;
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

	struct Sol sol;
	sol_init(&sol, card_createallshuf());

	struct UiSelection sel = { .place = SOL_STOCK, .card = NULL };
	SolCardPlace mv = 0;

	do {
		if (mv)
			draw_sol_with_mv(stdscr, sol, sel, mv);
		else
			ui_drawsol(stdscr, sol, sel);
		refresh();
	} while( handle_key(&sol, &sel, &mv, getch()) );

	sol_free(sol);
	endwin();

	return 0;
}
