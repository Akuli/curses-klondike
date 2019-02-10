#include "help.h"
#include <assert.h>
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "card.h"     // for SuitColor enum
#include "misc.h"
#include "scroll.h"

// copy/pasted from man page, i don't know why i get warnings without this
int mvwaddnwstr(WINDOW *win, int y, int x, const wchar_t *wstr, int n);

static const char *picture[] = {
	"╭──╮╭──╮    ╭──╮╭──╮╭──╮╭──╮",
	"│  ││  │    │ foundations  │",
	"╰──╯╰──╯    ╰──╯╰──╯╰──╯╰──╯",
	"  │   ╰─── discard          ",
	"  ╰─── stock                ",
	"╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮",
	"│         tableau          │",
	"╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯"
};

#define PICTURE_WIDTH (mbstowcs(NULL, picture[0], 0))
#define PICTURE_HEIGHT (sizeof(picture)/sizeof(picture[0]))

// note: there's a %s in the rules, that should be substituted with argv[0]
static const char rules[] =
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

// return value must be free()d
static char *get_rules(const char *argv0)
{
	int len = snprintf(NULL, 0, rules, argv0);
	assert(len > 0);

	char *buf = malloc(len+1);
	if (!buf)
		fatal_error("malloc() failed");

	int n = snprintf(buf, len+1, rules, argv0);
	assert(n == len);
	return buf;
}

static int get_max_width(int w, int xoff, int yoff)
{
	if (yoff > (int)PICTURE_HEIGHT)
		return w - xoff;
	return w - xoff - PICTURE_WIDTH - 3;  // 3 is for more space between picture and helps
}

// return value must be free()d
static wchar_t *string_to_wstring(const char *s)
{
	size_t n = mbstowcs(NULL, s, 0) + 1;
	wchar_t *ws = malloc(n * sizeof(wchar_t));
	if (!ws)
		fatal_error("malloc() failed");

	size_t m = mbstowcs(ws, s, n);
	assert(m == n-1);
	return ws;
}

static void print_colored(WINDOW *win, int y, int x, const wchar_t *s, int n, bool color)
{
	if (!win)
		return;

	for (int i = 0; i < n; i++) {
		int attr = 0;
		if (color)
			switch(s[i]) {
			case L'♥':
			case L'♦':
				attr = COLOR_PAIR(SUITCOLOR_RED);
				break;
			case L'♠':
			case L'♣':
				attr = COLOR_PAIR(SUITCOLOR_BLACK);
				break;
			default:
				break;
			}

		if (attr)
			wattron(win, attr);
		mvwaddnwstr(win, y, x+i, s+i, 1);
		if (attr)
			wattroff(win, attr);
	}
}

static void print_wrapped_colored(WINDOW *win, int w, const char *s, int xoff, int *yoff, bool color)
{
	// unicode is easiest to do with wchars in this case
	// wchars work because posix wchar_t is 4 bytes
	// windows has two-byte wchars which is too small for some unicodes, lol
	assert(sizeof(wchar_t) >= 4);
	wchar_t *wsstart = string_to_wstring(s);
	wchar_t *ws = wsstart;

	while (*ws) {
		int slen = wcslen(ws);
		int maxw = get_max_width(w, xoff, *yoff);
		int end = min(slen, maxw);
		wchar_t *brk;      // must be >=ws and <=ws+end

		// can break at newline?
		brk = wmemchr(ws, L'\n', end+1);   // +1 includes ws[end]

		// can break at space?
		if (!brk && end != slen) {
			// can't use wcsrchr because could be: end != wcslen(ws)
			for (brk = ws+end; brk > ws; brk--)
				if (*brk == ' ')
					break;
			if (brk == ws)  // nothing found
				brk = NULL;
		}

		if (brk) {
			print_colored(win, (*yoff)++, xoff, ws, brk - ws, color);
			ws = brk+1;
		} else {
			print_colored(win, (*yoff)++, xoff, ws, end, color);
			ws += end;
		}
	}

	free(wsstart);
}

static int get_longest_key_length(void)
{
	static unsigned int res = 0;
	if (res == 0)
		for (const struct HelpKey *k = help_keys; k->key && k->desc; k++)
			res = max(res, mbstowcs(NULL, k->key, 0));
	return res;
}

static void print_help_item(WINDOW *win, int w, struct HelpKey help, int *y)
{
	int keymax = get_longest_key_length();
	int nspace = keymax - mbstowcs(NULL, help.key, 0);

	if (win)
		mvwprintw(win, *y, 0, "%*s%s:", nspace, "", help.key);

	print_wrapped_colored(win, w, help.desc, keymax + 2, y, false);
	//(*y)++;  // blank line
}

static void print_title(WINDOW *win, int w, const char *title, int *y)
{
	*y += 2;

	if (win) {
		int len = mbstowcs(NULL, title, 0);
		int x = (w - len)/2;
		mvwhline(win, *y, 0, 0, x-1);
		mvwaddstr(win, *y, x, title);
		mvwhline(win, *y, x+len+1, 0, w - (x+len+1));
	}

	*y += 2;
}

// returns number of lines
static int print_all_help(WINDOW *win, int w, const char *argv0, bool color)
{
	int picx = w - PICTURE_WIDTH;

	if (win)
		for (int y = 0; y < (int)PICTURE_HEIGHT; y++)
			mvwaddstr(win, y, picx, picture[y]);

	int y = 0;
	for (const struct HelpKey *k = help_keys; k->key && k->desc; k++)
		print_help_item(win, w, *k, &y);

	print_title(win, w, "Rules", &y);
	char *s = get_rules(argv0);
	print_wrapped_colored(win, w, s, 0, &y, color);
	free(s);

	return max(y, (int)PICTURE_HEIGHT);
}

void help_show(WINDOW *win, const char *argv0, bool color)
{
	int w, h;
	getmaxyx(win, h, w);
	(void) h;  // silence warning about unused var

	WINDOW *pad = newpad(print_all_help(NULL, w, argv0, false), w);
	if (!pad)
		fatal_error("newpad() failed");
	print_all_help(pad, w, argv0, color);

	scroll_showpad(win, pad);
}
