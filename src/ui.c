#include "ui.h"
#include <curses.h>
#include <stdio.h>
#include <string.h>
#include "card.h"
#include "misc.h"

#define XOFFSET 3
#define YOFFSET 2

static inline int x_cardcount2ui(int xcnt, int w)
{
	// evenly spaced 7 columns, centered
	//
	// space per column = w/7
	// center of column = space per column * (xcnt + 1/2)
	// result = center of column - UI_CARDWIDTH/2
	//
	// simplifying it gives this, i wrote it with just 1 division to get only 1
	// floordiv and hopefully less little errors, could have also used floats but
	// this works fine
	return (2*xcnt*w + w - 7*UI_CARDWIDTH)/(2*7);
}

static inline int y_cardcount2ui(int ycnt, int h)
{
	// 1 row above first row, then everything spaced with 1 blank row between
	// TODO: what if it overflows?
	return 1 + ycnt*(UI_CARDHEIGHT + 1);
}

// box() is annoyingly for subwindows only
static void draw_box(WINDOW *win, int x, int y, int w, int h, char bg)
{
	mvwaddch(win, y, x, ACS_ULCORNER);
	mvwhline(win, y, x+1, 0, UI_CARDWIDTH - 2);
	mvwaddch(win, y, x+UI_CARDWIDTH-1, ACS_URCORNER);

	mvwvline(win, y+1, x, 0, UI_CARDHEIGHT - 2);
	mvwvline(win, y+1, x+UI_CARDWIDTH-1, 0, UI_CARDHEIGHT - 2);

	mvwaddch(win, y+UI_CARDHEIGHT-1, x, ACS_LLCORNER);
	mvwhline(win, y+UI_CARDHEIGHT-1, x+1, 0, UI_CARDWIDTH - 2);
	mvwaddch(win, y+UI_CARDHEIGHT-1, x+UI_CARDWIDTH-1, ACS_LRCORNER);

	// fill the box with bg
	for (int i=1; i < w-1; i++)
		for (int j=1; j < h-1; j++)
			mvwaddch(win, y+j, x+i, bg);
}

// newwin() doesn't work because partially erasing borders is surprisingly tricky
// partial erasing is needed for cards that are on top of cards
// since we can't use subwindow borders, they're not very helpful
void ui_drawcard(WINDOW *win, struct Card crd, int xcnt, int ycnt, int xoff, int yoff)
{
	int w, h;
	getmaxyx(win, h, w);
	int x = x_cardcount2ui(xcnt, w) + xoff*XOFFSET;
	int y = y_cardcount2ui(ycnt, h) + yoff*YOFFSET;

	draw_box(win, x, y, UI_CARDWIDTH, UI_CARDHEIGHT, crd.visible ? ' ' : '?');

	if (crd.visible) {
		char sbuf[CARD_SUITSTRMAX], nbuf[CARD_NUMSTRMAX];
		card_suitstr(crd, sbuf);
		card_numstr(crd, nbuf);

		mvaddstr(y+1, x+1, nbuf);
		mvaddstr(y+1, x+UI_CARDWIDTH-2, sbuf);
		mvaddstr(y+UI_CARDHEIGHT-2, x+1, sbuf);
		mvaddstr(y+UI_CARDHEIGHT-2, x+UI_CARDWIDTH-1-strlen(nbuf), nbuf);
	}
}
