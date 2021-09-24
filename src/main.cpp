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
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

struct Game {
	Klondike klon;
	SelectionOrMove selmv;
	std::unique_ptr<std::array<Card, 13*4>> card_array;

	Game(std::unique_ptr<std::array<Card, 13*4>> card_array) : card_array(std::move(card_array)) {
		this->klon.init(*this->card_array);
		this->selmv.ismove = false;
		this->selmv.sel = Selection{ nullptr, CardPlace::stock() };
	}
};

static void handle_key(Game& game, int k, Args ar, const char *argv0)
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
		game = Game(std::move(game.card_array));
		break;

	case 's':
		if (!game.selmv.ismove) {
			game.klon.stock2discard(ar.pick);

			// if you change this, think about what if the discard card was selected?
			// then the moved card ended up on top of the old discarded card
			// and we have 2 cards selected, so you need to handle that
			game.selmv.select_top_card_at_place(game.klon, CardPlace::discard());
		}
		break;

	case 'd':
		if (!game.selmv.ismove)
			game.selmv.select_top_card_at_place(game.klon, CardPlace::discard());
		break;

	case 'f':
		if (!game.selmv.ismove && game.selmv.sel.card && game.klon.move2foundation(game.selmv.sel.card))
			game.selmv.select_top_card_at_place(game.klon, game.selmv.sel.place);
		break;
	case 'g':
		if (!game.selmv.ismove) {
			bool moved = game.klon.move2foundation(cardlist::top(game.klon.discard));
			if (!moved) {
				for (Card *tab : game.klon.tableau) {
					if (game.klon.move2foundation(cardlist::top(tab))) {
						moved = true;
						break;
					}
				}
			}
			if (moved)
				game.selmv.select_top_card_at_place(game.klon, game.selmv.sel.place);  // update sel.card if needed
		}
		break;

	case 27:   // esc key, didn't find KEY_ constant
		game.selmv.ismove = false;
		break;

	case KEY_UP:
	case KEY_DOWN:
		if (!game.selmv.ismove && k == KEY_UP && game.selmv.sel.more(game.klon))
			break;
		if (!game.selmv.ismove && k == KEY_DOWN && game.selmv.sel.less(game.klon))
			break;
		// fall through

	case KEY_LEFT:
	case KEY_RIGHT:
		game.selmv.select_another_card(game.klon, curses_key_to_seldirection(k));
		break;

	case KEY_PPAGE:
		if (!game.selmv.ismove) {
			while (game.selmv.sel.more(game.klon)) {}
		}
		break;

	case KEY_NPAGE:
		if (!game.selmv.ismove) {
			while (game.selmv.sel.less(game.klon)) {}
		}
		break;

	case '\n':
		if (game.selmv.ismove)
			game.selmv.end_move(game.klon);
		else if (game.selmv.sel.place == CardPlace::stock())
			game.klon.stock2discard(ar.pick);
		else if (game.selmv.sel.card && game.selmv.sel.card->visible)
			game.selmv.begin_move();
		break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		game.selmv.select_top_card_at_place(game.klon, CardPlace::tableau(k - '1'));
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

	Game game(std::make_unique<std::array<Card, 13*4>>());

	bool first = true;
	while(1) {
		ui_drawklon(stdscr, game.klon, game.selmv, args.color, args.discardhide);

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
		handle_key(game, k, args, argv[0]);
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
