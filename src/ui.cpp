#include "card.hpp"
#include "klon.hpp"
#include "ui.hpp"
#include <array>
#include <cassert>
#include <curses.h>
#include <stdexcept>
#include <string>

static constexpr int CARD_WIDTH = 7;
static constexpr int CARD_HEIGHT = 5;

// offsets for laying out cards so that they overlap
static constexpr int X_OFFSET = 3;
static constexpr int Y_OFFSET_SMALL = 1;
static constexpr int Y_OFFSET_BIG = 2;

void ui_initcolors()
{
	if (init_pair(SuitColor(SuitColor::RED).color_pair_number(), COLOR_RED, COLOR_BLACK) == ERR)
		throw std::runtime_error("init_color() failed");
	if (init_pair(SuitColor(SuitColor::BLACK).color_pair_number(), COLOR_WHITE, COLOR_BLACK) == ERR)
		throw std::runtime_error("init_color() failed");
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
	const std::string hrz, vrt, ul, ur, ll, lr;
};

// https://en.wikipedia.org/wiki/Box-drawing_character#Unicode
static const Border normal_border = { "─", "│", "╭", "╮", "╰", "╯" };
static const Border selected_border = { "═", "║", "╔", "╗", "╚", "╝" };

// box() is annoyingly for subwindows only
static void draw_box(WINDOW *win, int xstart, int ystart, int w, int h, char bg, bool sel)
{
	Border b = sel ? selected_border : normal_border;

	mvwaddstr(win, ystart, xstart, b.ul.c_str());
	for (int x=xstart+1; x < xstart+w-1; x++)
		mvwaddstr(win, ystart, x, b.hrz.c_str());
	mvwaddstr(win, ystart, xstart+w-1, b.ur.c_str());

	for (int y=ystart+1; y < ystart+h-1; y++) {
		mvwaddstr(win, y, xstart, b.vrt.c_str());
		for (int x=xstart+1; x < xstart+w-1; x++)
			mvwaddch(win, y, x, bg);
		mvwaddstr(win, y, xstart+w-1, b.vrt.c_str());
	}

	mvwaddstr(win, ystart+h-1, xstart, b.ll.c_str());
	for (int x=xstart+1; x < xstart+w-1; x++)
		mvwaddstr(win, ystart+h-1, x, b.hrz.c_str());
	mvwaddstr(win, ystart+h-1, xstart+w-1, b.lr.c_str());
}

// draws card on win
// xo and yo offsets as curses units, for drawing overlapping cards
//
// newwin() doesn't work because partially erasing borders is surprisingly tricky
// partial erasing is needed for cards that are on top of cards
// since we can't use subwindow borders, they're not very helpful
static void draw_card(WINDOW *win, const Card *card, int xstart, int ystart, bool sel, bool color)
{
	if (card || sel)
		draw_box(win, xstart, ystart, CARD_WIDTH, CARD_HEIGHT, (!card || card->visible) ? ' ' : '?', sel);
	if (!card)
		return;

	if (card->visible) {
		// underlying values of SuitColor are valid color pair numbers
		int attr = COLOR_PAIR(card->suit.color().color_pair_number());
		if (color)
			wattron(win, attr);
		mvaddstr(ystart+1, xstart+1, card->number_string().c_str());
		mvaddstr(ystart+1, xstart+CARD_WIDTH-2, card->suit.string().c_str());
		mvaddstr(ystart+CARD_HEIGHT-2, xstart+1, card->suit.string().c_str());
		mvaddstr(ystart+CARD_HEIGHT-2, xstart+CARD_WIDTH-1-card->number_string().length(), card->number_string().c_str());
		if (color)
			wattroff(win, attr);
	}
}

// unlike a simple for loop, handles overflow
static void draw_card_stack(WINDOW *win, const Card *botcrd, int xstart, int ystart, const Card *firstsel, bool color)
{
	if (!botcrd)
		return;

	int w, h;
	getmaxyx(win, h, w);
	(void)w;   // suppress warning about unused var

	// the text (number and suit) of botcrd is at ystart+1
	// let's figure out where it is for the topmost card
	int toptxty = ystart+1;
	int ncardstotal = 1;
	for (Card *card = botcrd->next /* botcrd is already counted */ ; card; card = card->next) {
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
	for (const Card *card = botcrd; card; card = card->next) {
		if (card == firstsel)
			sel = true;
		draw_card(win, card, xstart, y, sel, color);
		y += (--n > 0) ? Y_OFFSET_BIG : Y_OFFSET_SMALL;
	}
}

// https://github.com/Akuli/curses-klondike/issues/2
enum class DiscardHide { HIDE_ALL, SHOW_LAST_ONLY, SHOW_ALL };

// TODO: this function is quite long, break it up
static void draw_the_klon(WINDOW *win, const Klondike& klon, const Selection& sel, bool moving, bool color, DiscardHide dh, int dscxoff)
{
	werase(win);

	int w, h;
	getmaxyx(win, h, w);

	// drawing just one card is enough for this
	draw_card(win, klon.stock, ui_x(0, w), ui_y(0, h), sel.place == CardPlace::stock(), color);

	int nshowdis = klon.discardshow;
	if (sel.place == CardPlace::discard() && moving)  // user is moving a detached card, and it's in the discard
		nshowdis++;

	int x = ui_x(1, w);
	for (Card *card = cardlist::top_n(klon.discard, nshowdis); card; (card = card->next), (x += dscxoff)) {
		Card crdval = *card;

		assert(crdval.visible);
		if (dh == DiscardHide::HIDE_ALL)
			crdval.visible = false;
		if (dh == DiscardHide::SHOW_LAST_ONLY)
			crdval.visible = !card->next;

		draw_card(win, &crdval, x, ui_y(0, h), sel.place == CardPlace::discard() && !card->next, color);
	}

	if (!klon.discard)   // nothing was drawn, but if the discard is selected, at least draw that
		draw_card(win, nullptr, ui_x(1, w), ui_y(0, h), sel.place == CardPlace::discard(), color);

	// foundations are similar to discard
	for (int i=0; i < 4; i++)
		draw_card(win, cardlist::top(klon.foundations[i]), ui_x(3+i, w), ui_y(0, h), sel.place == CardPlace::foundation(i), color);

	// now the tableau... here we go
	for (int x=0; x < 7; x++) {
		if (!klon.tableau[x]) {
			// draw a border if the tableau item is selected
			draw_card(win, nullptr, ui_x(x, w), ui_y(1, h), sel.place == CardPlace::tableau(x), color);
			continue;
		}

		int yo = 0;
		for (Card *card = klon.tableau[x]; card; card = card->next) {
			if (card->visible) {
				draw_card_stack(win, card, ui_x(x, w), ui_y(1, h) + yo, sel.card, color);
				break;
			}

			draw_card(win, card, ui_x(x, w), ui_y(1, h) + yo, false, color);
			yo += Y_OFFSET_SMALL;
		}
	}

	if (klon.win()) {
		std::string msg = "you win :)";
		mvwaddstr(win, h/2, (w - msg.length())/2, msg.c_str());
	}
}

static DiscardHide decide_what_to_hide(const SelectionOrMove& selmv, bool cmdlnopt)
{
	if (!cmdlnopt)
		return DiscardHide::SHOW_ALL;

	if (!selmv.ismove)
		return DiscardHide::SHOW_LAST_ONLY;

	if (selmv.move.src == CardPlace::discard() && selmv.move.dest == CardPlace::discard())
		return DiscardHide::SHOW_LAST_ONLY;
	if (selmv.move.src == CardPlace::discard())
		return DiscardHide::HIDE_ALL;
	return DiscardHide::SHOW_LAST_ONLY;
}

void ui_drawklon(WINDOW *win, const Klondike& klon, const SelectionOrMove& selmv, bool color, bool discardhide)
{
	DiscardHide dh = decide_what_to_hide(selmv, discardhide);
	int dscxoff = discardhide ? 1 : X_OFFSET;

	if (selmv.ismove) {
		std::array<Card, 13*4> temp_cards;
		Klondike temp_klon;
		Selection temp_sel = { klon.dup(temp_klon, selmv.move.card, temp_cards), selmv.move.dest };
		temp_klon.move(temp_sel.card, temp_sel.place, true);
		draw_the_klon(win, temp_klon, temp_sel, true, color, dh, dscxoff);
	} else
		draw_the_klon(win, klon, selmv.sel, false, color, dh, dscxoff);
}
