#include "help.hpp"
#include <algorithm>
#include <assert.h>
#include <cursesw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "card.hpp"   // for SuitColor enum
#include "scroll.hpp"

static const std::vector<std::wstring> picture_lines = {
	L"╭──╮╭──╮    ╭──╮╭──╮╭──╮╭──╮",
	L"│  ││  │    │ foundations  │",
	L"╰──╯╰──╯    ╰──╯╰──╯╰──╯╰──╯",
	L"  │   ╰─── discard          ",
	L"  ╰─── stock                ",
	L"╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮",
	L"│         tableau          │",
	L"╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯"
};

// note: there's a %s in the rule_format, that should be substituted with argv[0]
static const wchar_t rule_format[] =
	L"Here the “suit” of a card means ♥, ♦, ♠ or ♣. "
	L"The “number” of a card means one of A,2,3,4,…,9,10,J,Q,K. "
	L"“Visible” cards are cards whose suit and number are visible to you, aka “face-up” cards.\n\n"

	L"The goal is to move all cards to foundations. "
	L"Each foundation can contain cards of the same suit in increasing order, starting at A. "
	L"For example, if you see ♥A in tableau or discard, you can move it to an empty foundation, and when you see ♥2, you can move it on top of the ♥A and so on.\n\n"

	L"Visible cards on tableau must be in decreasing order with altering colors. "
	L"For example, ♥J ♣10 ♦9 is valid, because the colors are red-black-red and the numbers are 11-10-9.\n\n"

	L"If all visible cards are moved away from a tableau place, the topmost non-visible card is flipped, so that it becomes visible. "
	L"Usually getting all those cards to flip is the most challenging thing in a klondike game. "
	L"If there are no non-visible cards left, the place becomes empty, and a K card can be moved to it.\n\n"

	L"Moving one or more cards from one tableau place to another is allowed. "
	L"Tableau cards can also be moved to foundations, but only one at a time.\n\n"

	L"You can use stock and discard at any time to get more possible moves. "
	L"Cards can be moved from stock to discard, and the topmost card in discard can be moved to tableau or to a foundation. "
	L"By default, 3 cards are moved from stock to discard if the stock contains 3 or more cards; otherwise, all stock cards are moved to discard. "
	L"This can be customized with the --pick option; for example, --pick=1 moves 3 cards instead of 1, which makes the game a lot easier.\n\n"

	L"Moving the topmost card of a foundation to tableau is also allowed. "
	L"This can be useful in some cases.\n\n"

	L"If the game is too hard or too easy, you can customize it with command-line options. "
	//                                this is a non-breaking space ----↓
	L"Quit this help and then the game by pressing q twice, and run “%s --help” to get a list of all supported options."
	;

static std::wstring get_rules(const char *argv0)
{
	std::wstring out;
	size_t n = swprintf(NULL, 0, rule_format, argv0) + 1;
	out.resize(n);
	swprintf(&out[0], n, rule_format, argv0);
	return out;
}

static int get_max_width(int w, int xoff, int yoff)
{
	if (yoff > (int)picture_lines.size())
		return w - xoff;
	return w - xoff - picture_lines[0].size() - 3;  // 3 is for more space between picture_lines and helps
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

static void print_wrapped_colored(WINDOW *win, int w, std::wstring ws, int xoff, int& yoff, bool color)
{
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
	int nspace = keymax - help.key.size();

	if (win)
		mvwprintw(win, y, 0, "%*s%s:", nspace, "", help.key.c_str());

	print_wrapped_colored(win, w, help.desc, keymax + 2, y, false);
}

static void print_title(WINDOW *win, int w, std::wstring title, int& y)
{
	y += 2;

	if (win) {
		int len = title.size();
		int x = (w - title.size())/2;
		mvwhline(win, y, 0, 0, x-1);
		mvwaddwstr(win, y, x, title.c_str());
		mvwhline(win, y, x+len+1, 0, w - (x+len+1));
	}

	y += 2;
}

// returns number of lines
static int print_all_help(WINDOW *win, int w, std::vector<HelpKey> hkeys, const char *argv0, bool color)
{
	if (win)
		for (int y = 0; y < (int)picture_lines.size(); y++)
			mvwaddwstr(win, y, w - picture_lines[y].size(), picture_lines[y].c_str());

	int keymax = 0;
	for (const HelpKey& k : hkeys)
		keymax = std::max(keymax, (int)k.key.size());

	int y = 0;
	for (const HelpKey& k : hkeys)
		print_help_item(win, w, keymax, k, y);

	print_title(win, w, L"Rules", y);
	print_wrapped_colored(win, w, get_rules(argv0), 0, y, color);

	return std::max(y, (int)picture_lines.size());
}

void help_show(WINDOW *win, std::vector<HelpKey> hkeys, const char *argv0, bool color)
{
	int w, h;
	getmaxyx(win, h, w);
	(void) h;  // silence warning about unused var

	WINDOW *pad = newpad(print_all_help(NULL, w, hkeys, argv0, false), w);
	if (!pad)
		throw std::runtime_error("newpad() failed");
	print_all_help(pad, w, hkeys, argv0, color);

	scroll_showpad(win, pad);
}
