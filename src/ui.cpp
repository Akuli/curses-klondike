#include "ui.hpp"
#include <assert.h>
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "card.hpp"
#include "klon.hpp"
#include "misc.hpp"

#define CARD_WIDTH 7
#define CARD_HEIGHT 5

// offsets for laying out cards so that they overlap
#define X_OFFSET 3
#define Y_OFFSET_SMALL 1
#define Y_OFFSET_BIG 2

void ui_initcolors(void)
{
	// underlying values of SuitColor are valid color pair numbers
	if (init_pair(SuitColor(SuitColor::RED).color_pair_number(), COLOR_RED, COLOR_BLACK) == ERR) fatal_error("init_color() failed");
	if (init_pair(SuitColor(SuitColor::BLACK).color_pair_number(), COLOR_WHITE, COLOR_BLACK) == ERR) fatal_error("init_color() failed");
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
	const char *hrz, *vrt, *ul, *ur, *ll, *lr;
};

// https://en.wikipedia.org/wiki/Box-drawing_character#Unicode
static const struct Border normal_border = { "─", "│", "╭", "╮", "╰", "╯" };
static const struct Border selected_border = { "═", "║", "╔", "╗", "╚", "╝" };

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
static void draw_card(WINDOW *win, const struct Card *crd, int xstart, int ystart, bool sel, bool color)
{
	if (crd || sel)
		draw_box(win, xstart, ystart, CARD_WIDTH, CARD_HEIGHT, (!crd || crd->visible) ? ' ' : '?', sel);
	if (!crd)
		return;

	if (crd->visible) {
		// underlying values of SuitColor are valid color pair numbers
		int attr = COLOR_PAIR(crd->suit.color().color_pair_number());
		if (color)
			wattron(win, attr);
		mvaddstr(ystart+1, xstart+1, crd->numstring().c_str());
		mvaddstr(ystart+1, xstart+CARD_WIDTH-2, crd->suit.string().c_str());
		mvaddstr(ystart+CARD_HEIGHT-2, xstart+1, crd->suit.string().c_str());
		mvaddstr(ystart+CARD_HEIGHT-2, xstart+CARD_WIDTH-1-crd->numstring().length(), crd->numstring().c_str());
		if (color)
			wattroff(win, attr);
	}
}

// unlike a simple for loop, handles overflow
static void draw_card_stack(WINDOW *win, const struct Card *botcrd, int xstart, int ystart, const struct Card *firstsel, bool color)
{
	if (!botcrd)
		return;

	int w, h;
	getmaxyx(win, h, w);
	(void)w;   // suppress warning about unused var

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
	while (toptxty >= h) {
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
	for (const struct Card *crd = botcrd; crd; crd = crd->next) {
		if (crd == firstsel)
			sel = true;
		draw_card(win, crd, xstart, y, sel, color);
		y += (--n > 0) ? Y_OFFSET_BIG : Y_OFFSET_SMALL;
	}
}

// https://github.com/Akuli/curses-klondike/issues/2
enum DiscardHide { DH_HIDE_ALL, DH_SHOW_LAST_ONLY, DH_SHOW_ALL };

// TODO: this function is quite long, break it up
static void draw_the_klon(WINDOW *win, struct Klon kln, struct Sel sel, bool moving, bool color, enum DiscardHide dh, int dscxoff)
{
	werase(win);

	int w, h;
	getmaxyx(win, h, w);

	// drawing just one card is enough for this
	draw_card(win, kln.stock, ui_x(0, w), ui_y(0, h), sel.place == KLON_STOCK, color);

	unsigned int nshowdis = kln.discardshow;
	if (sel.place == KLON_DISCARD && moving)  // user is moving a detached card, and it's in the discard
		nshowdis++;

	int x = ui_x(1, w);
	for (struct Card *crd = card_tops(kln.discard, nshowdis); crd; (crd = crd->next), (x += dscxoff)) {
		struct Card crdval = *crd;

		assert(crdval.visible);
		if (dh == DH_HIDE_ALL)
			crdval.visible = false;
		if (dh == DH_SHOW_LAST_ONLY)
			crdval.visible = !crd->next;

		draw_card(win, &crdval, x, ui_y(0, h), sel.place == KLON_DISCARD && !crd->next, color);
	}

	if (!kln.discard)   // nothing was drawn, but if the discard is selected, at least draw that
		draw_card(win, NULL, ui_x(1, w), ui_y(0, h), sel.place == KLON_DISCARD, color);

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
		const char *msg = "you win :)";
		mvwaddstr(win, h/2, (w - strlen(msg))/2, msg);
	}
}

static enum DiscardHide decide_what_to_hide(struct SelMv selmv, bool cmdlnopt)
{
	if (!cmdlnopt)
		return DH_SHOW_ALL;

	if (!selmv.ismv)
		return DH_SHOW_LAST_ONLY;

	if (selmv.mv.src == KLON_DISCARD && selmv.mv.dst == KLON_DISCARD)
		return DH_SHOW_LAST_ONLY;
	if (selmv.mv.src == KLON_DISCARD)
		return DH_HIDE_ALL;
	return DH_SHOW_LAST_ONLY;
}

void ui_drawklon(WINDOW *win, struct Klon kln, struct SelMv selmv, bool color, bool discardhide)
{
	enum DiscardHide dh = decide_what_to_hide(selmv, discardhide);
	int dscxoff = discardhide ? 1 : X_OFFSET;

	if (selmv.ismv) {
		struct Klon tmpkln;
		struct Sel tmpsel = { klon_dup(kln, &tmpkln, selmv.mv.card), selmv.mv.dst };

		klon_move(&tmpkln, tmpsel.card, tmpsel.place, true);
		draw_the_klon(win, tmpkln, tmpsel, true, color, dh, dscxoff);
	} else
		draw_the_klon(win, kln, selmv.sel, false, color, dh, dscxoff);
}
