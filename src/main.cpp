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

static SelDirection curses_key_to_seldirection(int k)
{
	switch(k) {
	case KEY_LEFT: return SelDirection::LEFT;
	case KEY_RIGHT: return SelDirection::RIGHT;
	case KEY_UP: return SelDirection::UP;
	case KEY_DOWN: return SelDirection::DOWN;
	default: throw std::logic_error("not arrow key");
	}
}


// help is externed in help.h
static const std::vector<HelpKey> help_keys = {
	{ L"h", L"show this help" },
	{ L"q", L"quit" },
	{ L"n", L"new game" },
	{ L"s", L"move card(s) from stock to discard and select discard" },
	{ L"d", L"select discard" },
	{ L"f", L"move selected card to any foundation, if possible" },
	{ L"g", L"move any card to any foundation, if possible" },
	{ L"Enter", L"start moving the selected card(s), or complete the move if currently moving" },
	{ L"Esc", L"if currently moving card(s), stop that" },
	{ L"←,→", L"move selection left/right" },
	{ L"↑,↓", L"move selection up/down or select more/less tableau cards" },
	{ L"PageUp", L"select all tableau cards" },
	{ L"PageDown", L"select only 1 tableau card" },
	{ L"1,2,…,7", L"select tableau by number" },
};

static void new_game(Klon *kln, SelMv *selmv)
{
	kln->init();
	selmv->ismv = false;
	selmv->sel.place = CardPlace::stock();
	selmv->sel.card = NULL;
}

// returns whether to continue playing
static bool handle_key(Klon *kln, SelMv *selmv, int k, Args ar, const char *argv0)
{
	if (k == 'h') {
		help_show(stdscr, help_keys, argv0, ar.color);
		return true;
	}

	if (k == 'q')
		return false;

	if (k == 'n')
		new_game(kln, selmv);

	if (k == 's' && !selmv->ismv) {
		kln->stock2discard(ar.pick);

		// if you change this, think about what if the discard card was selected?
		// then the moved card ended up on top of the old discarded card
		// and we have 2 cards selected, so you need to handle that
		selmv_byplace(*kln, selmv, CardPlace::discard());

		return true;
	}

	if (k == 'd' && !selmv->ismv)
		selmv_byplace(*kln, selmv, CardPlace::discard());

	if (k == 'f' && !selmv->ismv && selmv->sel.card) {
		if (kln->move2foundation(selmv->sel.card))
			selmv_byplace(*kln, selmv, selmv->sel.place);  // updates selmv->sel.card if needed
		return true;
	}

	if (k == 'g' && !selmv->ismv) {
		// inefficient, but not noticably inefficient
		if (kln->move2foundation(card_top(kln->discard)))
			goto moved;
		for (int i = 0; i < 7; i++)
			if (kln->move2foundation(card_top(kln->tableau[i])))
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
		else if (selmv->sel.place == CardPlace::stock())
			kln->stock2discard(ar.pick);
		else if (selmv->sel.card && selmv->sel.card->visible)
			selmv_beginmv(selmv);
		return true;
	}

	if ('1' <= k && k <= '7') {
		selmv_byplace(*kln, selmv, CardPlace::tableau(k - '1'));
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

	Klon kln;
	SelMv selmv;
	new_game(&kln, &selmv);

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
	} while( handle_key(&kln, &selmv, getch(), ar, argv[0]) );  // TODO: too many args

	endwin();

	return 0;
}
