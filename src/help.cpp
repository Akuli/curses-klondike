#include "card.hpp"
#include "help.hpp"
#include "scroll.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cursesw.h>
#include <memory>
#include <stdexcept>

static constexpr std::array<std::string_view, 8> picture_lines = {
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

static std::wstring string_to_wstring(const std::string& s)
{
	size_t n = mbstowcs(nullptr, s.c_str(), 0) + 1;
	assert(n != (size_t)-1);

	std::wstring out;
	out.resize(n+1);
	mbstowcs(out.data(), s.c_str(), n+1);
	return out;
}

static int get_max_width(int w, int xoff, int yoff)
{
	if (yoff > (int)picture_lines.size())
		return w - xoff;
	// 3 is for more space between picture_lines and helps
	return w - xoff - string_to_wstring(std::string(picture_lines[0])).length() - 3;
}

class Printer {
public:
	int y = 0;
	void reset(WINDOW *w) { this->y = 0; this->window = w; }

	int print_all_help(int w, std::vector<HelpKey> hkeys, const char *argv0, bool color)
	{
		if (this->window) {
			for (int y = 0; y < (int)picture_lines.size(); y++) {
				std::string line = std::string(picture_lines[y]);
				mvwaddstr(this->window, y, w - string_to_wstring(line).length(), line.c_str());
			}
		}

		int keymax = 0;
		for (const HelpKey& k : hkeys)
			keymax = std::max(keymax, (int)string_to_wstring(k.key).length());

		for (const HelpKey& k : hkeys)
			this->print_help_item(w, keymax, k);

		this->print_title(w, "Rules");
		this->print_wrapped_colored(w, get_rules(argv0).c_str(), 0, color);
		return std::max(y, (int)picture_lines.size());
	}

private:
	WINDOW *window = NULL;

	void print_colored(int x, const std::wstring& s, bool color) const
	{
		if (!this->window)
			return;

		for (wchar_t w : s) {
			int attr = 0;
			if (color)
				switch(w) {
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
			mvwaddnwstr(this->window, this->y, x++, &w, 1);
			if (attr)
				wattroff(this->window, attr);
		}
	}

	void print_wrapped_colored(int w, const std::string& s, int xoff, bool color)
	{
		// unicode is easiest to do with wchars in this case
		// wchars work because posix wchar_t is 4 bytes
		// windows has two-byte wchars which is too small for some unicodes, lol
		static_assert(sizeof(wchar_t) >= 4);
		std::wstring ws = string_to_wstring(s);

		while (!ws.empty()) {
			int maxw = get_max_width(w, xoff, this->y);
			int end = std::min((int)ws.length(), maxw);

			// can break at newline?
			size_t brk = ws.substr(0, end).find(L'\n');

			// can break at space?
			if (brk == std::wstring::npos && end < (int)ws.length())
				brk = ws.substr(0, end).find_last_of(L' ');

			this->print_colored(xoff, ws.substr(0, brk), color);
			this->y++;

			if (brk == std::wstring::npos)
				ws = ws.substr(end);
			else
				ws = ws.substr(brk+1);
		}
	}

	void print_help_item(int w, int keymax, const HelpKey& help)
	{
		int nspace = keymax - string_to_wstring(help.key).length();

		if (this->window)
			mvwprintw(this->window, y, 0, "%*s%s:", nspace, "", help.key.c_str());

		this->print_wrapped_colored(w, help.desc, keymax + 2, false);
	}

	void print_title(int w, const std::string& title)
	{
		this->y += 2;

		if (this->window) {
			int len = string_to_wstring(title).length();
			int x = (w - len)/2;
			mvwhline(this->window, y, 0, 0, x-1);
			mvwaddstr(this->window, y, x, title.c_str());
			mvwhline(this->window, y, x+len+1, 0, w - (x+len+1));
		}

		this->y += 2;
	}
};

void help_show(WINDOW *window, std::vector<HelpKey> hkeys, const char *argv0, bool color)
{
	int width, height;
	getmaxyx(window, height, width);
	(void) height;  // silence warning about unused var

	Printer printer;
	printer.print_all_help(width, hkeys, argv0, false);
	WINDOW *pad = newpad(printer.y, width);
	if (!pad)
		throw std::runtime_error("newpad() failed");

	printer.reset(pad);
	printer.print_all_help(width, hkeys, argv0, color);
	scroll_showpad(window, pad);
}
