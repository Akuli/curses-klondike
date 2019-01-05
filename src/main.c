// for setenv(3)
#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "args.h"
#include "card.h"
#include "help.h"
#include "misc.h"
#include "sel.h"
#include "klon.h"
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

// these are externed in args.h
FILE *args_outfile;
FILE *args_errfile;


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


// help is externed in help.h
struct Help help[] = {
	{ "h", "show this help" },
	{ "q", "quit" },
	{ "n", "new game" },
	{ "s", "move a card from stock to discard and select discard" },
	{ "d", "select discard" },
	{ "f", "move selected card to a foundation, if possible" },
	{ "Enter", "start moving the selected card(s)" },
	{ "↑,↓", "select more/less tableau cards or move selection up/down" },
	{ "←,→", "move selection left/right" },
	{ "1,2,…,7", "select tableau by number" },
	{ NULL, NULL }
};

static void new_game(struct Klon *kln, struct UiSelection *sel, KlonCardPlace *mv)
{
	klon_init(kln, card_createallshuf());
	sel->place = KLON_STOCK;
	sel->card = NULL;
	*mv = 0;
}

// returns whether to continue playing
static bool handle_key(struct Klon *kln, struct UiSelection *sel, KlonCardPlace *mv, int k, struct Args ar)
{
	if (k == 'h') {
		help_show(stdscr);
		return true;
	}

	if (k == 'q')
		return false;

	if (k == 'n') {
		klon_free(*kln);
		new_game(kln, sel, mv);
	}

	if (k == 's' && !*mv) {
		klon_stock2discard(kln, ar.pick);

		// if you change this, think about what if the discard card was selected?
		// then the moved card ended up on top of the old discarded card
		// and we have 2 cards selected, so you need to handle that
		sel_byplace(*kln, sel, KLON_DISCARD);

		return true;
	}

	if (k == 'd') {
		if (*mv)
			*mv = KLON_DISCARD;
		else
			sel_byplace(*kln, sel, KLON_DISCARD);
	}

	if (k == 'f' && sel->card && !*mv) {
		for (int i=0; i < 4; i++)
			if (klon_canmove(*kln, sel->card, KLON_FOUNDATION(i))) {
				klon_move(kln, sel->card, KLON_FOUNDATION(i));
				sel_byplace(*kln, sel, sel->place);  // updates sel->card if needed
				break;
			}
		return true;
	}

	if (k == 27) {   // esc key, didn't find a KEY_ constant for this
		if (*mv)
			*mv = 0;
		return true;
	}

	if (k == KEY_LEFT || k == KEY_RIGHT || k == KEY_UP || k == KEY_DOWN) {
		if (*mv)
			sel_anothercardmv(*kln, *sel, curses_key_to_seldirection(k), mv);
		else {
			if (k == KEY_UP && sel_more(*kln, sel))
				return true;
			if (k == KEY_DOWN && sel_less(*kln, sel))
				return true;
			sel_anothercard(*kln, sel, curses_key_to_seldirection(k));
		}
		return true;
	}

	if (k == '\n') {
		if (*mv) {
			sel_endmv(kln, sel, *mv);
			*mv = 0;
		}
		else if (sel->place == KLON_STOCK)
			klon_stock2discard(kln, ar.pick);
		else if (sel->card && sel->card->visible)
			*mv = sel->place;
		return true;
	}

	if ('1' <= k && k <= '7') {
		if (*mv)
			*mv = KLON_TABLEAU(k - '1');
		else
			sel_byplace(*kln, sel, KLON_TABLEAU(k - '1'));
		return true;
	}

	return true;
}

// creates a temp copy of the kln, modifies it nicely and calls ui_drawkln
static void draw_klon_with_mv(WINDOW *win, struct Klon kln, struct UiSelection sel, KlonCardPlace mv, bool color)
{
	struct Klon tmpkln;
	struct UiSelection tmpsel = { .place = mv };

	tmpsel.card = klon_dup(kln, &tmpkln, sel.card);
	klon_rawmove(&tmpkln, tmpsel.card, tmpsel.place);

	ui_drawklon(win, tmpkln, tmpsel, !!mv, color);
	klon_free(tmpkln);
}

int main(int argc, char **argv)
{
	// displaying unicodes correctly needs setlocale here AND cursesw instead of curses in makefile
	if (!setlocale(LC_ALL, ""))
		fatal_error("setlocale() failed");

	args_outfile = stdout;
	args_errfile = stderr;
	struct Args ar;
	int sts = args_parse(&ar, argc, argv);
	if (sts >= 0)
		return sts;

	// https://stackoverflow.com/a/28020568
	// see also ESCDELAY in a man page named "ncurses"
	// setting to "0" works, but feels like a hack, so i used same as in stackoverflow
	// TODO: add a configure script to allow compiling without setenv()?
	if (setenv("ESCDELAY", "25", false) < 0)
		fatal_error("setenv() failed");

	time_t t = time(NULL);
	if (t == (time_t)(-1))
		fatal_error("time() failed");
	srand(t);

	if (!initscr())
		fatal_error("initscr() failed");
	initscred = true;

	bool color = (ar.color && has_colors() && start_color() != ERR);
	if (color)
		ui_initcolors();

	if (cbreak() == ERR) fatal_error("cbreak() failed");
	if (curs_set(0) == ERR) fatal_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR) fatal_error("keypad() failed");

	refresh();   // yes, this is needed before drawing the cards for some reason

	struct Klon kln;
	struct UiSelection sel;
	KlonCardPlace mv;
	new_game(&kln, &sel, &mv);

	bool first = true;
	do {
		if (mv)
			draw_klon_with_mv(stdscr, kln, sel, mv, color);
		else
			ui_drawklon(stdscr, kln, sel, !!mv, color);

		if (first) {
			wattron(stdscr, COLOR_PAIR(2));
			mvwaddstr(stdscr, getmaxy(stdscr) - 1, 0, "Press h for help.");
			wattroff(stdscr, COLOR_PAIR(2));
			first = false;
		}

		refresh();
	} while( handle_key(&kln, &sel, &mv, getch(), ar) );

	klon_free(kln);
	endwin();

	return 0;
}
