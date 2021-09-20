#include "scroll.hpp"
#include <algorithm>
#include <curses.h>
#include <limits.h>
#include <stdbool.h>

#define BOTTOM_BAR_SIZE 1

struct ScrollState {
	WINDOW *const win;
	WINDOW *const pad;
	int firstlineno = 0;   // 0 means first line
};

// makes sure that it's not scrolled too far up or down
static void bounds_check(ScrollState *st)
{
	int winh, padh, w;
	getmaxyx(st->win, winh, w);
	getmaxyx(st->pad, padh, w);
	winh -= BOTTOM_BAR_SIZE;
	(void) w;  // w is needed only because getmaxyx wants it, this suppresses warning

	// it's important that the negativeness check is last
	// this way firstlineno is set to 0 if padh < winh
	if (st->firstlineno > padh - winh)
		st->firstlineno = padh - winh;
	if (st->firstlineno < 0)
		st->firstlineno = 0;
}

static void draw_pad_to_window(ScrollState *st)
{
	bounds_check(st);

	int winw, winh, padw, padh;
	getmaxyx(st->win, winh, winw);
	getmaxyx(st->pad, padh, padw);
	winh -= BOTTOM_BAR_SIZE;

	wclear(st->win);  // werase() doesn't fill window with dark background in color mode

	// min stuff and -1 are needed because curses is awesome
	// if this code is wrong, it either segfaults or does nothing
	copywin(st->pad, st->win, st->firstlineno, 0, 0, 0, std::min(winh, padh)-1, std::min(winw, padw)-1, true);

	const char *msg;
	if (winh < padh)
		msg = "Move with ↑ and ↓, or press q to quit.";
	else
		msg = "Press q to quit.";

	wattron(st->win, A_STANDOUT);
	mvwaddstr(st->win, winh, 0, msg);
	wattroff(st->win, A_STANDOUT);

	wrefresh(st->win);
}

static bool handle_key(ScrollState *st, int k)
{
	int winw, winh;
	getmaxyx(st->win, winh, winw);
	winh -= BOTTOM_BAR_SIZE;
	(void) winw;

	switch(k) {
	case 'q':
		return false;

	case KEY_UP:
	case 'p':
		st->firstlineno--;
		break;

	case KEY_DOWN:
	case 'n':
		st->firstlineno++;
		break;

	case KEY_PPAGE:
		st->firstlineno -= winh;
		break;

	case KEY_NPAGE:
	case ' ':
		st->firstlineno += winh;
		break;

	case KEY_HOME:
		st->firstlineno = 0;
		break;

	case KEY_END:
		st->firstlineno = INT_MAX;
		break;

	default:
		break;
	}

	return true;
}

// must wrefresh(win) after this, but not before
void scroll_showpad(WINDOW *win, WINDOW *pad)
{
	ScrollState st = { win, pad };
	do {
		werase(win);
		draw_pad_to_window(&st);
		wrefresh(win);
	} while( handle_key(&st, getch()) );
}
