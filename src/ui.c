#include "ui.h"
#include <curses.h>
#include <stdio.h>
#include <string.h>
#include "card.h"
#include "misc.h"
#include "sol.h"

// offsets for laying out cards so that they overlap
#define X_OFFSET 3
#define Y_OFFSET_SMALL 1
#define Y_OFFSET_BIG 2

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
	// start at first row, then no blank rows in between
	// because the line drawing characters look blanky enough anyway
	// TODO: add "..." or something if it goes too far
	return ycnt*UI_CARDHEIGHT;
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

// draws crd on win
// xo, yob, yos are for drawing cards so that they partially overlap
// xo = x offset, yos = small y offset, yob = guess what
//
// newwin() doesn't work because partially erasing borders is surprisingly tricky
// partial erasing is needed for cards that are on top of cards
// since we can't use subwindow borders, they're not very helpful
static void draw_card(WINDOW *win, struct Card crd, int xcnt, int ycnt, int xo, int yos, int yob)
{
	int w, h;
	getmaxyx(win, h, w);
	int x = x_cardcount2ui(xcnt, w) + xo*X_OFFSET;
	int y = y_cardcount2ui(ycnt, h) + yos*Y_OFFSET_SMALL + yob*Y_OFFSET_BIG;

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

void ui_drawsol(WINDOW *win, struct Sol sol)
{
	// all cards in stock are non-visible and perfectly lined up on top of each other
	// so just draw one of them, if any
	if (sol.stock)
		draw_card(win, *sol.stock, 0, 0, 0, 0, 0);

	// discard contains lined-up cards, but they're lined up again, so only last one can show
	if (sol.discard)
		draw_card(win, *card_top(sol.discard), 1, 0, 0, 0, 0);

	// foundations are similar to discard
	for (int i=0; i < 4; i++)
		if (sol.foundations[i])
			draw_card(win, *card_top(sol.foundations[i]), 3+i, 0, 0, 0, 0);

	// now the tableau... here we go
	for (int x=0; x < 7; x++) {
		int yos = 0, yob = 0;
		for (struct Card *crd = sol.tableau[x]; crd; crd = crd->next) {
			if (crd->visible)
				draw_card(win, *crd, x, 1, 0, yos, yob++);
			else
				draw_card(win, *crd, x, 1, 0, yos++, yob);
		}
	}
}
