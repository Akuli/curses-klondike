#include "card.hpp"
#include "help.hpp"
#include "scroll.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cursesw.h>
#include <stdexcept>
#include <string>

static constexpr std::array<const char *, 8> picture_lines = {
	"╭──╮╭──╮    ╭──╮╭──╮╭──╮╭──╮",
	"│  ││  │    │ foundations  │",
	"╰──╯╰──╯    ╰──╯╰──╯╰──╯╰──╯",
	"  │   ╰─── discard          ",
	"  ╰─── stock                ",
	"╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮",
	"│         tableau          │",
	"╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯"
};

static std::string get_rules(const char *argv0)
{
	std::string parts[2] = {
		"Here the “suit” of a card means ♥, ♦, ♠ or ♣. "
		"The “number” of a card means one of A,2,3,4,…,9,10,J,Q,K. "
		"“Visible” cards are cards whose suit and number are visible to you, aka “face-up” cards.\n\n"

		"The goal is to move all cards to foundations. "
		"Each foundation can contain cards of the same suit in increasing order, starting at A. "
		"For example, if you see ♥A in tableau or discard, you can move it to an empty foundation, and when you see ♥2, you can move it on top of the ♥A and so on.\n\n"

		"Visible cards on tableau must be in decreasing order with altering colors. "
		"For example, ♥J ♣10 ♦9 is valid, because the colors are red-black-red and the numbers are 11-10-9.\n\n"

		"If all visible cards are moved away from a tableau place, the topmost non-visible card is flipped, so that it becomes visible. "
		"Usually getting all those cards to flip is the most challenging thing in a klondike game. "
		"If there are no non-visible cards left, the place becomes empty, and a K card can be moved to it.\n\n"

		"Moving one or more cards from one tableau place to another is allowed. "
		"Tableau cards can also be moved to foundations, but only one at a time.\n\n"

		"You can use stock and discard at any time to get more possible moves. "
		"Cards can be moved from stock to discard, and the topmost card in discard can be moved to tableau or to a foundation. "
		"By default, 3 cards are moved from stock to discard if the stock contains 3 or more cards; otherwise, all stock cards are moved to discard. "
		"This can be customized with the --pick option; for example, --pick=1 moves 3 cards instead of 1, which makes the game a lot easier.\n\n"

		"Moving the topmost card of a foundation to tableau is also allowed. "
		"This can be useful in some cases.\n\n"

		"If the game is too hard or too easy, you can customize it with command-line options. "
		//                                 this is a non-breaking space ----↓
		"Quit this help and then the game by pressing q twice, and run “", " --help” to get a list of all supported options."
	};
	return parts[0] + argv0 + parts[1];
}

static std::wstring cstring_to_wstring(const char *utf8_string)
{
	size_t wide_length = mbstowcs(nullptr, utf8_string, 0);
	assert(wide_length != (size_t)-1);

	std::wstring result;
	result.resize(wide_length + 1);
	mbstowcs(result.data(), utf8_string, wide_length + 1);
	return result;
}

class Printer {
public:
	int terminal_width;
	int y = 0;
	Printer(int width, bool color) : terminal_width(width), color(color) {}
	void reset(WINDOW *window) { this->y = 0; this->window = window; }

	int print_all_help(std::vector<HelpItem> help_items, const char *argv0)
	{
		if (this->window) {
			for (int y = 0; y < (int)picture_lines.size(); y++) {
				const char *line = picture_lines[y];
				mvwaddstr(this->window, y, this->terminal_width - cstring_to_wstring(line).length(), line);
			}
		}

		int max_key_len = 0;
		for (const HelpItem& item : help_items)
			max_key_len = std::max(max_key_len, (int)cstring_to_wstring(std::string(item.key).c_str()).length());

		for (const HelpItem& item : help_items)
			this->print_help_item(max_key_len, item);

		this->print_title("Rules");
		this->print_wrapped_colored(get_rules(argv0), 0);
		return std::max(this->y, (int)picture_lines.size());
	}

private:
	bool color;
	WINDOW *window = NULL;

	void print_colored(int x, std::wstring_view string) const
	{
		if (!this->window)
			return;

		for (wchar_t character : string) {
			int attr = 0;
			if (this->color)
				switch(character) {
				case L'♥':
				case L'♦':
					attr = COLOR_PAIR(SuitColor(SuitColor::RED).color_pair_number());
					break;
				case L'♠':
				case L'♣':
					attr = COLOR_PAIR(SuitColor(SuitColor::BLACK).color_pair_number());
					break;
				default:
					break;
				}

			if (attr)
				wattron(this->window, attr);
			mvwaddnwstr(this->window, this->y, x++, &character, 1);
			if (attr)
				wattroff(this->window, attr);
		}
	}

	void print_wrapped_colored(const std::string& utf8_string, int x_offset)
	{
		// unicode is easiest to do with wchars in this case
		// wchars work because posix wchar_t is 4 bytes
		// windows has two-byte wchars which is too small for some unicodes, lol
		static_assert(sizeof(wchar_t) >= 4);
		std::wstring wide_string = cstring_to_wstring(utf8_string.c_str());
		std::wstring_view view = wide_string;

		while (!view.empty()) {
			int max_width = this->terminal_width - x_offset;
			if (this->y <= (int)picture_lines.size()) {
				max_width -= cstring_to_wstring(picture_lines[0]).length();
				max_width -= 3;  // more space between picture_lines and helps
			}

			int end = std::min((int)view.length(), max_width);

			// can break at newline?
			size_t line_break = view.substr(0, end).find(L'\n');

			// can break at space?
			if (line_break == std::wstring::npos && end < (int)view.length())
				line_break = view.substr(0, end).find_last_of(L' ');

			this->print_colored(x_offset, view.substr(0, line_break));
			this->y++;

			if (line_break == std::wstring::npos)
				view = view.substr(end);
			else
				view = view.substr(line_break+1);
		}
	}

	void print_help_item(int max_key_len, const HelpItem& item)
	{
		int space_count = max_key_len - cstring_to_wstring(std::string(item.key).c_str()).length();

		if (this->window) {
			mvwprintw(this->window, this->y, 0,
				"%*s%.*s:",
				space_count, "",
				item.key.length(), item.key.data());
		}

		this->print_wrapped_colored(std::string(item.desc), max_key_len + 2);
	}

	void print_title(const std::string& title)
	{
		this->y += 2;

		int text_width = cstring_to_wstring(title.c_str()).length();
		int x = (this->terminal_width - text_width)/2;
		int left = x-1;
		int right = x+text_width+1;

		if (this->window) {
			mvwhline(this->window, this->y, 0, 0, left);
			mvwaddstr(this->window, this->y, x, title.c_str());
			mvwhline(this->window, this->y, right, 0, this->terminal_width - right);
		}

		this->y += 2;
	}
};

void help_show(WINDOW *window, std::vector<HelpItem> help_items, const char *argv0, bool color)
{
	int width, height;
	getmaxyx(window, height, width);
	(void) height;  // silence warning about unused var

	Printer printer = {width, color};
	printer.print_all_help(help_items, argv0);  // calculate height

	WINDOW *pad = newpad(printer.y, width);
	if (!pad)
		throw std::runtime_error("newpad() failed");

	printer.reset(pad);
	printer.print_all_help(help_items, argv0);
	scroll_showpad(window, pad);

	delwin(pad);
}
