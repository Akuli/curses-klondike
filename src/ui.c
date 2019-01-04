#include "ui.h"
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "card.h"
#include "klon.h"
#include "misc.h"

#define CARD_WIDTH 7
#define CARD_HEIGHT 5

// offsets for laying out cards so that they overlap
#define X_OFFSET 3
#define Y_OFFSET_SMALL 1
#define Y_OFFSET_BIG 2

#define RED_SUIT_PAIR 1
#define BLACK_SUIT_PAIR 2

void ui_initcolors(void)
{
	if (init_pair(RED_SUIT_PAIR, COLOR_RED, COLOR_BLACK) == ERR) fatal_error("init_color() failed");
	if (init_pair(BLACK_SUIT_PAIR, COLOR_WHITE, COLOR_BLACK) == ERR) fatal_error("init_color() failed");
}

// ui_x() and ui_y() convert coordinates from card counts to curses coordinates
static inline int ui_x(int xcnt, int w)
{
	// evenly spaced 7 columns, centered
	//
	// space per column = w/7
	// center of column = space per column * (xcnt + 1/2)
	// result = center of column - CARD_WIDTH/2
	//
	// simplifying it gives this, i wrote it with just 1 division to get only 1
	// floordiv and hopefully less little errors, could have also used floats but
	// this works fine
	return (2*xcnt*w + w - 7*CARD_WIDTH)/(2*7);
}

static inline int ui_y(int ycnt, int h)
{
	// start at first row, then no blank rows in between
	// because the line drawing characters look blanky enough anyway
	return ycnt*CARD_HEIGHT;
}

// because i need 2 kind of borders, "normal" and "selected"
struct Border {
	// e.g. hrz = horizontal, ul = upper left
	char *hrz, *vrt, *ul, *ur, *ll, *lr;
};

// https://en.wikipedia.org/wiki/Box-drawing_character#Unicode
static struct Border normal_border = { "─", "│", "╭", "╮", "╰", "╯" };
static struct Border selected_border = { "═", "║", "╔", "╗", "╚", "╝" };

// box() is annoyingly for subwindows only
static void draw_box(WINDOW *win, int xstart, int ystart, int w, int h, char bg, bool sel)
{
	struct Border b = sel ? selected_border : normal_border;

	mvwaddstr(win, ystart, xstart, b.ul);
	for (int x=xstart+1; x < xstart+w-1; x++)
		mvwaddstr(win, ystart, x, b.hrz);
	mvwaddstr(win, ystart, xstart+w-1, b.ur);

	for (int y=ystart+1; y < ystart+h-1; y++) {
		mvwaddstr(win, y, xstart, b.vrt);
		for (int x=xstart+1; x < xstart+w-1; x++)
			mvwaddch(win, y, x, bg);
		mvwaddstr(win, y, xstart+w-1, b.vrt);
	}

	mvwaddstr(win, ystart+h-1, xstart, b.ll);
	for (int x=xstart+1; x < xstart+w-1; x++)
		mvwaddstr(win, ystart+h-1, x, b.hrz);
	mvwaddstr(win, ystart+h-1, xstart+w-1, b.lr);
}

// draws crd on win
// xo and yo offsets as curses units, for drawing overlapping cards
//
// newwin() doesn't work because partially erasing borders is surprisingly tricky
// partial erasing is needed for cards that are on top of cards
// since we can't use subwindow borders, they're not very helpful
static void draw_card(WINDOW *win, struct Card *crd, int xstart, int ystart, bool sel, bool color)
{
	if (crd || sel)
		draw_box(win, xstart, ystart, CARD_WIDTH, CARD_HEIGHT, (!crd || crd->visible) ? ' ' : '?', sel);
	if (!crd)
		return;

	if (crd->visible) {
		char sbuf[CARD_SUITSTRMAX], nbuf[CARD_NUMSTRMAX];
		card_suitstr(*crd, sbuf);
		card_numstr(*crd, nbuf);

		int attr = COLOR_PAIR(SUIT_COLOR(crd->suit) ? RED_SUIT_PAIR : BLACK_SUIT_PAIR);
		if (color)
			wattron(win, attr);
		mvaddstr(ystart+1, xstart+1, nbuf);
		mvaddstr(ystart+1, xstart+CARD_WIDTH-2, sbuf);
		mvaddstr(ystart+CARD_HEIGHT-2, xstart+1, sbuf);
		mvaddstr(ystart+CARD_HEIGHT-2, xstart+CARD_WIDTH-1-strlen(nbuf), nbuf);
		if (color)
			wattroff(win, attr);
	}
}

// unlike a simple for loop, handles overflow
static void draw_card_stack(WINDOW *win, struct Card *botcrd, int xstart, int ystart, struct Card *firstsel, bool color)
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
	bool sel = false;
	int y = ystart;
	for (struct Card *crd = botcrd; crd; crd = crd->next) {
		if (crd == firstsel)
			sel = true;
		draw_card(win, crd, xstart, y, sel, color);
		y += (--n > 0) ? Y_OFFSET_BIG : Y_OFFSET_SMALL;
	}
}

void ui_drawklon(WINDOW *win, struct Klon kln, struct UiSelection sel, bool color)
{
	werase(win);

	int w, h;
	getmaxyx(win, h, w);

	// all cards in stock are non-visible and perfectly lined up on top of each other
	// so just draw one of them, if any
	draw_card(win, kln.stock, ui_x(0, w), ui_y(0, h), sel.place == KLON_STOCK, color);

	// discard contains lined-up cards, too
	draw_card(win, card_top(kln.discard), ui_x(1, w), ui_y(0, h), sel.place == KLON_DISCARD, color);

	// foundations are similar to discard
	for (int i=0; i < 4; i++)
		draw_card(win, card_top(kln.foundations[i]), ui_x(3+i, w), ui_y(0, h), sel.place == KLON_FOUNDATION(i), color);

	// now the tableau... here we go
	for (int x=0; x < 7; x++) {
		if (!kln.tableau[x]) {
			// draw a border if the tableau item is selected
			draw_card(win, NULL, ui_x(x, w), ui_y(1, h), sel.place == KLON_TABLEAU(x), color);
			continue;
		}

		int yo = 0;
		for (struct Card *crd = kln.tableau[x]; crd; crd = crd->next) {
			if (crd->visible) {
				draw_card_stack(win, crd, ui_x(x, w), ui_y(1, h) + yo, sel.card, color);
				break;
			}

			draw_card(win, crd, ui_x(x, w), ui_y(1, h) + yo, false, color);
			yo += Y_OFFSET_SMALL;
		}
	}

	if (klon_win(kln)) {
		char *msg = "you win :)";
		mvwaddstr(win, h/2, (w - strlen(msg))/2, msg);
	}
}
