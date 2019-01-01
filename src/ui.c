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

// ui_x() and ui_y() convert coordinates from card counts to curses coordinates
static inline int ui_x(int xcnt, int w)
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

static inline int ui_y(int ycnt, int h)
{
	// start at first row, then no blank rows in between
	// because the line drawing characters look blanky enough anyway
	// TODO: add "..." or something if it goes too far
	return ycnt*UI_CARDHEIGHT;
}

// box() is annoyingly for subwindows only
static void draw_box(WINDOW *win, int xstart, int ystart, int w, int h, char bg)
{
	mvwaddch(win, ystart, xstart, ACS_ULCORNER);
	mvwhline(win, ystart, xstart+1, 0, UI_CARDWIDTH - 2);
	mvwaddch(win, ystart, xstart+UI_CARDWIDTH-1, ACS_URCORNER);

	mvwvline(win, ystart+1, xstart, 0, UI_CARDHEIGHT - 2);
	mvwvline(win, ystart+1, xstart+UI_CARDWIDTH-1, 0, UI_CARDHEIGHT - 2);

	mvwaddch(win, ystart+UI_CARDHEIGHT-1, xstart, ACS_LLCORNER);
	mvwhline(win, ystart+UI_CARDHEIGHT-1, xstart+1, 0, UI_CARDWIDTH - 2);
	mvwaddch(win, ystart+UI_CARDHEIGHT-1, xstart+UI_CARDWIDTH-1, ACS_LRCORNER);

	// fill the box with bg
	for (int x = xstart+1; x < xstart+w-1; x++)
		for (int y = ystart+1; y < ystart+h-1; y++)
			mvwaddch(win, y, x, bg);
}

// draws crd on win
// xo and yo offsets as curses units, for drawing overlapping cards
//
// newwin() doesn't work because partially erasing borders is surprisingly tricky
// partial erasing is needed for cards that are on top of cards
// since we can't use subwindow borders, they're not very helpful
static void draw_card(WINDOW *win, struct Card crd, int xstart, int ystart)
{

	draw_box(win, xstart, ystart, UI_CARDWIDTH, UI_CARDHEIGHT, crd.visible ? ' ' : '?');

	if (crd.visible) {
		char sbuf[CARD_SUITSTRMAX], nbuf[CARD_NUMSTRMAX];
		card_suitstr(crd, sbuf);
		card_numstr(crd, nbuf);

		mvaddstr(ystart+1, xstart+1, nbuf);
		mvaddstr(ystart+1, xstart+UI_CARDWIDTH-2, sbuf);
		mvaddstr(ystart+UI_CARDHEIGHT-2, xstart+1, sbuf);
		mvaddstr(ystart+UI_CARDHEIGHT-2, xstart+UI_CARDWIDTH-1-strlen(nbuf), nbuf);
	}
}

// unlike a simple for loop, handles overflow
static void draw_card_stack(WINDOW *win, struct Card *botcrd, int xstart, int ystart)
{
	if (!botcrd)
		return;

	// the text (num and suit) of botcrd is at ystart+1
	// let's figure out where it is for the topmost card
	int toptxty = ystart+1;
	int ncardstotal = 1;
	for (struct Card *crd = botcrd->next /* botcrd is already counted */ ; crd; crd = crd->next) {
		toptxty += Y_OFFSET_BIG;
		ncardstotal++;
	}

	// we can make all cards visible by displaying some cards with a smaller offset
	// we'll display n cards with the bigger offset
	int n = ncardstotal;
	while (toptxty >= getmaxy(win)) {
		toptxty -= Y_OFFSET_BIG;
		toptxty += Y_OFFSET_SMALL;
		n--;
	}

	// to give some extra room that wouldn't be really necessary, but is nicer
	// without the if, cards get stacked even when there's enough room
	if (n != ncardstotal)
		n--;

	// let's draw the cards
	for (struct Card *crd = botcrd; crd; crd = crd->next) {
		draw_card(win, *crd, xstart, ystart);
		ystart += (--n > 0) ? Y_OFFSET_BIG : Y_OFFSET_SMALL;
	}
}

void ui_drawsol(WINDOW *win, struct Sol sol)
{
	werase(win);

	int w, h;
	getmaxyx(win, h, w);

	// all cards in stock are non-visible and perfectly lined up on top of each other
	// so just draw one of them, if any
	if (sol.stock)
		draw_card(win, *sol.stock, ui_x(0, w), ui_y(0, h));

	// discard contains lined-up cards, but they're lined up again, so only last one can show
	if (sol.discard)
		draw_card(win, *card_top(sol.discard), ui_x(1, w), ui_y(0, h));

	// foundations are similar to discard
	for (int i=0; i < 4; i++)
		if (sol.foundations[i])
			draw_card(win, *card_top(sol.foundations[i]), ui_x(3+i, w), ui_y(0, h));

	// now the tableau... here we go
	for (int x=0; x < 7; x++) {
		int yo = 0;
		for (struct Card *crd = sol.tableau[x]; crd; crd = crd->next) {
			if (crd->visible) {
				draw_card_stack(win, crd, ui_x(x, w), ui_y(1, h) + yo);
				break;
			}

			draw_card(win, *crd, ui_x(x, w), ui_y(1, h) + yo);
			yo += Y_OFFSET_SMALL;
		}
	}
}
