#define _POSIX_C_SOURCE 200112L  // for setenv(3)

#include "args.hpp"
#include "card.hpp"
#include "help.hpp"
#include "klon.hpp"
#include "selectmove.hpp"
#include "ui.hpp"
#include <algorithm>
#include <array>
#include <clocale>
#include <cstdlib>
#include <ctime>
#include <cursesw.h>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


static SelDirection curses_key_to_seldirection(int key)
{
	switch(key) {
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

	void handle_key(int key, Args parsed_args, const char *argv0)
	{
		switch(key) {
		case 'h':
			show_help(
				stdscr,
				std::vector<HelpItem>(help_items.begin(), help_items.end()),
				argv0,
				parsed_args.color);
			break;

		case 's':
			if (!this->selmv.ismove) {
				this->klon.stock2discard(parsed_args.pick);

				// if you change this, think about what if the discard card was selected?
				// then the moved card ended up on top of the old discarded card
				// and we have 2 cards selected, so you need to handle that
				this->selmv.select_top_card_or_move_to(this->klon, CardPlace::discard());
			}
			break;

		case 'd':
			if (!this->selmv.ismove)
				this->selmv.select_top_card_or_move_to(this->klon, CardPlace::discard());
			break;

		case 'f':
			if (!this->selmv.ismove && this->selmv.sel.card && this->klon.move2foundation(this->selmv.sel.card))
				this->selmv.select_top_card_or_move_to(this->klon, this->selmv.sel.place);
			break;
		case 'g':
			if (!this->selmv.ismove) {
				bool moved =
					this->klon.move2foundation(cardlist::top(this->klon.discard))
					|| std::any_of(this->klon.tableau.begin(), this->klon.tableau.end(), [&](Card *list){
						return this->klon.move2foundation(cardlist::top(list));
					});
				if (moved)
					this->selmv.select_top_card_or_move_to(this->klon, this->selmv.sel.place);  // update sel.card if needed
			}
			break;

		case 27:   // esc key, didn't find KEY_ constant
			this->selmv.ismove = false;
			break;

		case KEY_UP:
		case KEY_DOWN:
			if (!this->selmv.ismove && key == KEY_UP && this->selmv.sel.select_more(this->klon))
				break;
			if (!this->selmv.ismove && key == KEY_DOWN && this->selmv.sel.select_more(this->klon))
				break;
			// fall through

		case KEY_LEFT:
		case KEY_RIGHT:
			this->selmv.select_another_card(this->klon, curses_key_to_seldirection(key));
			break;

		case KEY_PPAGE:
			if (!this->selmv.ismove) {
				while (this->selmv.sel.select_more(this->klon)) {}
			}
			break;

		case KEY_NPAGE:
			if (!this->selmv.ismove) {
				while (this->selmv.sel.select_more(this->klon)) {}
			}
			break;

		case '\n':
			if (this->selmv.ismove)
				this->selmv.end_move(this->klon);
			else if (this->selmv.sel.place == CardPlace::stock())
				this->klon.stock2discard(parsed_args.pick);
			else if (this->selmv.sel.card && this->selmv.sel.card->visible)
				this->selmv.begin_move();
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			this->selmv.select_top_card_or_move_to(this->klon, CardPlace::tableau(key - '1'));
			break;

		default:
			break;
		}
	}
};


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

static int main_internal(int argc, const char *const *argv)
{
	// displaying unicodes correctly needs setlocale here AND cursesw instead of curses in makefile
	if (!setlocale(LC_ALL, ""))
		throw std::runtime_error("setlocale() failed");
	std::srand(std::time(nullptr));

	int status;
	std::optional<Args> args_option = parse_args(
		status, std::vector<std::string>(argv, argv + argc),
		std::cout, std::cerr);
	if (!args_option)
		return status;
	Args parsed_args = args_option.value();

	// https://stackoverflow.com/a/28020568
	// see also ESCDELAY in a man page named "ncurses"
	// setting to "0" works, but feels like a hack, so i used same as in stackoverflow
	if (setenv("ESCDELAY", "25", false) < 0)
		throw std::runtime_error("setenv() failed");

	CursesSession ses;

	parsed_args.color = (parsed_args.color && has_colors() && start_color() != ERR);
	if (parsed_args.color)
		Suit::init_color_pairs();

	if (cbreak() == ERR) throw std::runtime_error("cbreak() failed");
	if (noecho() == ERR) throw std::runtime_error("noecho() failed");
	if (curs_set(0) == ERR) throw std::runtime_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR) throw std::runtime_error("keypad() failed");

	Game game(std::make_unique<std::array<Card, 13*4>>());

	bool first = true;
	while(1) {
		draw_klondike(stdscr, game.klon, game.selmv, parsed_args);

		if (first) {
			int width, height;
			getmaxyx(stdscr, height, width);
			(void) width;   // silence warning about unused variable

			wattron(stdscr, A_STANDOUT);
			mvwaddstr(stdscr, height-1, 0, "Press h for help.");
			wattroff(stdscr, A_STANDOUT);
			first = false;
		}

		refresh();
		int key = getch();
		if (key == 'q')
			return 0;
		else if (key == 'n')
			game = Game(std::move(game.card_array));
		else
			game.handle_key(key, parsed_args, argv[0]);
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
