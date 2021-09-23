// TODO: noecho
#define _POSIX_C_SOURCE 200112L  // for setenv(3)

#include "args.hpp"
#include "card.hpp"
#include "help.hpp"
#include "klon.hpp"
#include "selectmove.hpp"
#include "ui.hpp"
#include <array>
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

static constexpr std::array<HelpItem, 14> help_items = {
	HelpItem{ "h", "show this help" },
	HelpItem{ "q", "quit" },
	HelpItem{ "n", "new game" },
	HelpItem{ "s", "move card(s) from stock to discard and select discard" },
	HelpItem{ "d", "select discard" },
	HelpItem{ "f", "move selected card to any foundation, if possible" },
	HelpItem{ "g", "move any card to any foundation, if possible" },
	HelpItem{ "Enter", "start moving the selected card(s), or complete the move if currently moving" },
	HelpItem{ "Esc", "if currently moving card(s), stop that" },
	HelpItem{ "←,→", "move selection left/right" },
	HelpItem{ "↑,↓", "move selection up/down or select more/less tableau cards" },
	HelpItem{ "PageUp", "select all tableau cards" },
	HelpItem{ "PageDown", "select only 1 tableau card" },
	HelpItem{ "1,2,…,7", "select tableau by number" },
};

static void new_game(Klondike &klon, SelectionOrMove &selmv)
{
	klon.init();
	selmv.ismove = false;
	selmv.sel = Selection{ nullptr, CardPlace::stock() };
}

static void handle_key(Klondike& klon, SelectionOrMove& selmv, int k, Args ar, const char *argv0)
{
	switch(k) {
	case 'h':
		help_show(
			stdscr,
			std::vector<HelpItem>(help_items.begin(), help_items.end()),
			argv0,
			ar.color);
		break;

	case 'n':
		new_game(klon, selmv);
		break;

	case 's':
		if (!selmv.ismove) {
			klon.stock2discard(ar.pick);

			// if you change this, think about what if the discard card was selected?
			// then the moved card ended up on top of the old discarded card
			// and we have 2 cards selected, so you need to handle that
			selmv.select_top_card_at_place(klon, CardPlace::discard());
		}
		break;

	case 'd':
		if (!selmv.ismove)
			selmv.select_top_card_at_place(klon, CardPlace::discard());
		break;

	case 'f':
		if (!selmv.ismove && selmv.sel.card && klon.move2foundation(selmv.sel.card))
			selmv.select_top_card_at_place(klon, selmv.sel.place);
		break;
	case 'g':
		if (!selmv.ismove) {
			bool moved = klon.move2foundation(cardlist::top(klon.discard));
			if (!moved) {
				for (Card *tab : klon.tableau) {
					if (klon.move2foundation(cardlist::top(tab))) {
						moved = true;
						break;
					}
				}
			}
			if (moved)
				selmv.select_top_card_at_place(klon, selmv.sel.place);  // updates selmv.sel.card if needed
		}
		break;

	case 27:   // esc key, didn't find KEY_ constant
		selmv.ismove = false;
		break;

	case KEY_UP:
	case KEY_DOWN:
		if (!selmv.ismove && k == KEY_UP && selmv.sel.more(klon))
			break;
		if (!selmv.ismove && k == KEY_DOWN && selmv.sel.less(klon))
			break;
		// fall through

	case KEY_LEFT:
	case KEY_RIGHT:
		selmv.select_another_card(klon, curses_key_to_seldirection(k));
		break;

	case KEY_PPAGE:
		if (!selmv.ismove) {
			while (selmv.sel.more(klon)) {}
		}
		break;

	case KEY_NPAGE:
		if (!selmv.ismove) {
			while (selmv.sel.less(klon)) {}
		}
		break;

	case '\n':
		if (selmv.ismove)
			selmv.end_move(klon);
		else if (selmv.sel.place == CardPlace::stock())
			klon.stock2discard(ar.pick);
		else if (selmv.sel.card && selmv.sel.card->visible)
			selmv.begin_move();
		break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		selmv.select_top_card_at_place(klon, CardPlace::tableau(k - '1'));
		break;

	default:
		break;
	}
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

	std::vector<std::string> arg_vector = {};
	for (int i = 0; i < argc; i++)
		arg_vector.push_back(argv[i]);

	int status;
	std::optional<Args> args_option = args_parse(status, arg_vector, stdout, stderr);
	if (!args_option.has_value())
		return status;
	Args args = args_option.value();

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
	args.color = (args.color && has_colors() && start_color() != ERR);
	if (args.color)
		ui_initcolors();

	if (cbreak() == ERR) throw std::runtime_error("cbreak() failed");
	if (curs_set(0) == ERR) throw std::runtime_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR) throw std::runtime_error("keypad() failed");

	refresh();   // yes, this is needed before drawing the cards for some reason

	Klondike klon;
	SelectionOrMove selmv;
	new_game(klon, selmv);

	bool first = true;
	while(1) {
		ui_drawklon(stdscr, klon, selmv, args.color, args.discardhide);

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
		int k = getch();
		if (k == 'q')
			return 0;
		handle_key(klon, selmv, k, args, argv[0]);  // TODO: too many args
	}
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
