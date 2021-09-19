// TODO: noecho
// for setenv(3)
#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "args.hpp"
#include "card.hpp"
#include "help.hpp"
#include "misc.hpp"
#include "selmv.hpp"
#include "klon.hpp"
#include "ui.hpp"


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


// help is externed in help.h
static const std::vector<HelpKey> help_keys = {
	{ "h", "show this help" },
	{ "q", "quit" },
	{ "n", "new game" },
	{ "s", "move card(s) from stock to discard and select discard" },
	{ "d", "select discard" },
	{ "f", "move selected card to any foundation, if possible" },
	{ "g", "move any card to any foundation, if possible" },
	{ "Enter", "start moving the selected card(s), or complete the move if currently moving" },
	{ "Esc", "if currently moving card(s), stop that" },
	{ "←,→", "move selection left/right" },
	{ "↑,↓", "move selection up/down or select more/less tableau cards" },
	{ "PageUp", "select all tableau cards" },
	{ "PageDown", "select only 1 tableau card" },
	{ "1,2,…,7", "select tableau by number" },
};

static void new_game(Klon *kln, SelMv *selmv, std::unique_ptr<Card[]>& cardlist)
{
	klon_init(*kln);
	selmv->ismv = false;
	selmv->sel.place = KLON_STOCK;
	selmv->sel.card = NULL;
}

// returns whether to continue playing
static bool handle_key(Klon *kln, SelMv *selmv, int k, Args ar, const char *argv0, std::unique_ptr<Card[]>& cardlist)
{
	if (k == 'h') {
		help_show(stdscr, help_keys, argv0, ar.color);
		return true;
	}

	if (k == 'q')
		return false;

	if (k == 'n')
		new_game(kln, selmv, cardlist);

	if (k == 's' && !selmv->ismv) {
		klon_stock2discard(kln, ar.pick);

		// if you change this, think about what if the discard card was selected?
		// then the moved card ended up on top of the old discarded card
		// and we have 2 cards selected, so you need to handle that
		selmv_byplace(*kln, selmv, KLON_DISCARD);

		return true;
	}

	if (k == 'd' && !selmv->ismv)
		selmv_byplace(*kln, selmv, KLON_DISCARD);

	if (k == 'f' && !selmv->ismv && selmv->sel.card) {
		if (klon_move2foundation(kln, selmv->sel.card))
			selmv_byplace(*kln, selmv, selmv->sel.place);  // updates selmv->sel.card if needed
		return true;
	}

	if (k == 'g' && !selmv->ismv) {
		// inefficient, but not noticably inefficient
		if (klon_move2foundation(kln, card_top(kln->discard)))
			goto moved;
		for (int i = 0; i < 7; i++)
			if (klon_move2foundation(kln, card_top(kln->tableau[i])))
				goto moved;
		return true;

	moved:
		selmv_byplace(*kln, selmv, selmv->sel.place);  // updates selmv->sel.card if needed
		return true;
	}

	if (k == 27) {   // esc key, didn't find a KEY_ constant for this
		if (selmv->ismv)
			selmv->ismv = false;
		return true;
	}

	if (k == KEY_LEFT || k == KEY_RIGHT || k == KEY_UP || k == KEY_DOWN) {
		if (!selmv->ismv) {
			if (k == KEY_UP && sel_more(*kln, &selmv->sel))
				return true;
			if (k == KEY_DOWN && sel_less(*kln, &selmv->sel))
				return true;
		}

		selmv_anothercard(*kln, selmv, curses_key_to_seldirection(k));
		return true;
	}

	if ((k == KEY_PPAGE || k == KEY_NPAGE) && !selmv->ismv) {
		bool (*func)(Klon, Sel *) = (k==KEY_PPAGE ? sel_more : sel_less);
		while (func(*kln, &selmv->sel))
			;
		return true;
	}

	if (k == '\n') {
		if (selmv->ismv)
			selmv_endmv(kln, selmv);
		else if (selmv->sel.place == KLON_STOCK)
			klon_stock2discard(kln, ar.pick);
		else if (selmv->sel.card && selmv->sel.card->visible)
			selmv_beginmv(selmv);
		return true;
	}

	if ('1' <= k && k <= '7') {
		selmv_byplace(*kln, selmv, KLON_TABLEAU(k - '1'));
		return true;
	}

	return true;
}

int main(int argc, char **argv)
{
	// displaying unicodes correctly needs setlocale here AND cursesw instead of curses in makefile
	if (!setlocale(LC_ALL, ""))
		fatal_error("setlocale() failed");

	std::vector<std::string> argvec = {};
	for (int i = 0; i < argc; i++)
		argvec.push_back(argv[i]);

	Args ar;
	int sts = args_parse(ar, argvec, stdout, stderr);
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

	// changing ar.color here makes things easier
	ar.color = (ar.color && has_colors() && start_color() != ERR);
	if (ar.color)
		ui_initcolors();

	if (cbreak() == ERR) fatal_error("cbreak() failed");
	if (curs_set(0) == ERR) fatal_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR) fatal_error("keypad() failed");

	refresh();   // yes, this is needed before drawing the cards for some reason

	std::unique_ptr<Card[]> cardlist = nullptr;
	Klon kln;
	SelMv selmv;
	new_game(&kln, &selmv, cardlist);

	bool first = true;
	do {
		ui_drawklon(stdscr, kln, selmv, ar.color, ar.discardhide);

		if (first) {
			int w, h;
			getmaxyx(stdscr, h, w);
			(void) w;   // silence warning about unused variable

			wattron(stdscr, A_STANDOUT);
			mvwaddstr(stdscr, h-1, 0, "Press h for help.");
			wattroff(stdscr, A_STANDOUT);
			first = false;
		}

		refresh();
	} while( handle_key(&kln, &selmv, getch(), ar, argv[0], cardlist) );  // TODO: too many args

	endwin();

	return 0;
}