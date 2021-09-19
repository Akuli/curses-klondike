#include "help.hpp"
#include <algorithm>
#include <assert.h>
#include <cursesw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "card.hpp"   // for SuitColor enum
#include "misc.hpp"
#include "scroll.hpp"

static const std::vector<std::string> picture_lines = {
	"╭──╮╭──╮    ╭──╮╭──╮╭──╮╭──╮",
	"│  ││  │    │ foundations  │",
	"╰──╯╰──╯    ╰──╯╰──╯╰──╯╰──╯",
	"  │   ╰─── discard          ",
	"  ╰─── stock                ",
	"╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮",
	"│         tableau          │",
	"╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯"
};

static const int picture_width = mbstowcs(NULL, picture_lines[0].c_str(), 0);

// note: there's a %s in the rule_format, that should be substituted with argv[0]
static const char rule_format[] =
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
	//                               this is a non-breaking space ----↓
	"Quit this help and then the game by pressing q twice, and run “%s --help” to get a list of all supported options."
	;

static std::string get_rules(const char *argv0)
{
	std::string out;
	out.resize(snprintf(NULL, 0, rule_format, argv0) + 1);
	sprintf(&out[0], rule_format, argv0);
	return out;
}

static std::wstring string_to_wstring(std::string s)
{
	size_t n = mbstowcs(NULL, s.c_str(), 0) + 1;
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
	return w - xoff - string_to_wstring(picture_lines[0]).size() - 3;  // 3 is for more space between picture_lines and helps
}

static void print_colored(WINDOW *win, int y, int x, std::wstring s, bool color)
{
	if (!win)
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
			wattron(win, attr);
		mvwaddnwstr(win, y, x++, &w, 1);
		if (attr)
			wattroff(win, attr);
	}
}

static void print_wrapped_colored(WINDOW *win, int w, std::string s, int xoff, int& yoff, bool color)
{
	// unicode is easiest to do with wchars in this case
	// wchars work because posix wchar_t is 4 bytes
	// windows has two-byte wchars which is too small for some unicodes, lol
	static_assert(sizeof(wchar_t) >= 4);
	std::wstring ws = string_to_wstring(s);

	while (!ws.empty()) {
		int maxw = get_max_width(w, xoff, yoff);
		int end = std::min((int)ws.size(), maxw);

		// can break at newline?
		size_t brk = ws.substr(0, end).find(L'\n');

		// can break at space?
		if (brk == std::wstring::npos && end < (int)ws.size())
			brk = ws.substr(0, end).find_last_of(L' ');

		print_colored(win, yoff++, xoff, ws.substr(0, brk), color);
		if (brk == std::wstring::npos)
			ws = ws.substr(end);
		else
			ws = ws.substr(brk+1);
	}
}

static void print_help_item(WINDOW *win, int w, int keymax, const HelpKey& help, int& y)
{
	int nspace = keymax - string_to_wstring(help.key).size();

	if (win)
		mvwprintw(win, y, 0, "%*s%s:", nspace, "", help.key.c_str());

	print_wrapped_colored(win, w, help.desc, keymax + 2, y, false);
}

static void print_title(WINDOW *win, int w, std::string title, int& y)
{
	y += 2;

	if (win) {
		int len = string_to_wstring(title).size();
		int x = (w - len)/2;
		mvwhline(win, y, 0, 0, x-1);
		mvwaddstr(win, y, x, title.c_str());
		mvwhline(win, y, x+len+1, 0, w - (x+len+1));
	}

	y += 2;
}

// returns number of lines
static int print_all_help(WINDOW *win, int w, std::vector<HelpKey> hkeys, const char *argv0, bool color)
{
	if (win)
		for (int y = 0; y < (int)picture_lines.size(); y++)
			mvwaddstr(win, y, w - string_to_wstring(picture_lines[y]).size(), picture_lines[y].c_str());

	int keymax = 0;
	for (const HelpKey& k : hkeys)
		keymax = std::max(keymax, (int)string_to_wstring(k.key).size());

	int y = 0;
	for (const HelpKey& k : hkeys)
		print_help_item(win, w, keymax, k, y);

	print_title(win, w, "Rules", y);
	print_wrapped_colored(win, w, get_rules(argv0).c_str(), 0, y, color);

	return std::max(y, (int)picture_lines.size());
}

void help_show(WINDOW *win, std::vector<HelpKey> hkeys, const char *argv0, bool color)
{
	int w, h;
	getmaxyx(win, h, w);
	(void) h;  // silence warning about unused var

	WINDOW *pad = newpad(print_all_help(NULL, w, hkeys, argv0, false), w);
	if (!pad)
		fatal_error("newpad() failed");
	print_all_help(pad, w, hkeys, argv0, color);

	scroll_showpad(win, pad);
}
