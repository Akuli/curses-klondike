// TODO: noecho
#define _POSIX_C_SOURCE 200112L  // for setenv(3)

#include "args.hpp"
#include "card.hpp"
#include "help.hpp"
#include "klon.hpp"
#include "selmv.hpp"
#include "ui.hpp"
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cursesw.h>
#include <stdexcept>
#include <string>
#include <vector>


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

static void new_game(Klon &kln, SelMv &selmv)
{
	kln.init();
	selmv.ismv = false;
	selmv.sel = Sel{ nullptr, CardPlace::stock() };
}

// returns whether to continue playing
static bool handle_key(Klon& kln, SelMv& selmv, int k, Args ar, const char *argv0)
{
	if (k == 'h') {
		help_show(stdscr, help_keys, argv0, ar.color);
		return true;
	}

	if (k == 'q')
		return false;

	if (k == 'n')
		new_game(kln, selmv);

	if (k == 's' && !selmv.ismv) {
		kln.stock2discard(ar.pick);

		// if you change this, think about what if the discard card was selected?
		// then the moved card ended up on top of the old discarded card
		// and we have 2 cards selected, so you need to handle that
		selmv.select_top_card_at_place(kln, CardPlace::discard());

		return true;
	}

	if (k == 'd' && !selmv.ismv)
		selmv.select_top_card_at_place(kln, CardPlace::discard());

	if (k == 'f' && !selmv.ismv && selmv.sel.card) {
		if (kln.move2foundation(selmv.sel.card))
			selmv.select_top_card_at_place(kln, selmv.sel.place);  // updates selmv.sel.card if needed
		return true;
	}

	if (k == 'g' && !selmv.ismv) {
		// inefficient, but not noticably inefficient
		if (kln.move2foundation(cardlist::top(kln.discard)))
			goto moved;
		for (int i = 0; i < 7; i++)
			if (kln.move2foundation(cardlist::top(kln.tableau[i])))
				goto moved;
		return true;

	moved:
		selmv.select_top_card_at_place(kln, selmv.sel.place);  // updates selmv.sel.card if needed
		return true;
	}

	if (k == 27) {   // esc key, didn't find a KEY_ constant for this
		if (selmv.ismv)
			selmv.ismv = false;
		return true;
	}

	if (k == KEY_LEFT || k == KEY_RIGHT || k == KEY_UP || k == KEY_DOWN) {
		if (!selmv.ismv) {
			if (k == KEY_UP && selmv.sel.more(kln))
				return true;
			if (k == KEY_DOWN && selmv.sel.less(kln))
				return true;
		}

		selmv.select_another_card(kln, curses_key_to_seldirection(k));
		return true;
	}

	if (k == KEY_PPAGE && !selmv.ismv) {
		while (selmv.sel.more(kln)) {}
		return true;
	}

	if (k == KEY_NPAGE && !selmv.ismv) {
		while (selmv.sel.less(kln)) {}
		return true;
	}

	if (k == '\n') {
		if (selmv.ismv)
			selmv.end_move(kln);
		else if (selmv.sel.place == CardPlace::stock())
			kln.stock2discard(ar.pick);
		else if (selmv.sel.card && selmv.sel.card->visible)
			selmv.begin_move();
		return true;
	}

	if ('1' <= k && k <= '7') {
		selmv.select_top_card_at_place(kln, CardPlace::tableau(k - '1'));
		return true;
	}

	return true;
}

class CursesSession {
public:
	CursesSession() {
		if (!initscr())
			throw std::runtime_error("initscr() failed");
	}
	~CursesSession() {
		endwin();
	}
};

static int main_internal(int argc, char **argv)
{
	// displaying unicodes correctly needs setlocale here AND cursesw instead of curses in makefile
	if (!setlocale(LC_ALL, ""))
		throw std::runtime_error("setlocale() failed");

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
	if (setenv("ESCDELAY", "25", false) < 0)
		throw std::runtime_error("setenv() failed");

	time_t t = time(nullptr);
	if (t == (time_t)(-1))
		throw std::runtime_error("time() failed");
	srand(t);

	CursesSession ses;

	// changing ar.color here makes things easier
	ar.color = (ar.color && has_colors() && start_color() != ERR);
	if (ar.color)
		ui_initcolors();

	if (cbreak() == ERR) throw std::runtime_error("cbreak() failed");
	if (curs_set(0) == ERR) throw std::runtime_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR) throw std::runtime_error("keypad() failed");

	refresh();   // yes, this is needed before drawing the cards for some reason

	Klon kln;
	SelMv selmv;
	new_game(kln, selmv);

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
	} while( handle_key(kln, selmv, getch(), ar, argv[0]) );  // TODO: too many args

	endwin();

	return 0;
}

int main(int argc, char **argv)
{
	// https://stackoverflow.com/q/222175
	try {
		return main_internal(argc, argv);
	} catch(...) {
		throw;
	}
}
