#include "ui.h"
#include <curses.h>
#include <stdio.h>
#include <string.h>
#include "card.h"
#include "misc.h"

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

void ui_drawcard(WINDOW *win, struct Card crd, int xcnt, int ycnt)
{
	int w, h;
	getmaxyx(win, h, w);
	int x = x_cardcount2ui(xcnt, w), y = y_cardcount2ui(ycnt, h);

	// TODO: don't create a new window for this every time?
	WINDOW *crdwin = newwin(UI_CARDHEIGHT, UI_CARDWIDTH, y, x);
	if (!crdwin)
		fatal_error("newwin() failed");

	box(crdwin, 0, 0);
	if (crd.visible) {
		char buf[CARD_STRMAX];
		card_str(crd, buf);
		mvwaddstr(crdwin, 1, 1, buf);
	}

	wrefresh(crdwin);
	delwin(crdwin);
}
