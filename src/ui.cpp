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
static inline int ui_x(int x_count, int terminal_width)
{
	// evenly spaced 7 columns, centered
	//
	// space per column = terminal_width/7
	// center of column = space per column * (x_count + 1/2)
	// result = center of column - CARD_WIDTH/2
	//
	// simplifying it gives this, i wrote it with just 1 division to get only 1
	// floordiv and hopefully less little errors, could have also used floats but
	// this works fine
	return ((2*x_count + 1)*terminal_width - 7*CARD_WIDTH)/(2*7);
}

static inline int ui_y(int y_count)
{
	// start at first row, then no blank rows in between
	// because the line drawing characters look blanky enough anyway
	return y_count*CARD_HEIGHT;
}

// https://en.wikipedia.org/wiki/Box-drawing_character#Unicode
struct Border {
	const char *horizontal, *vertical, *upper_left, *upper_right, *lower_left, *lower_right;
};
static constexpr Border normal_border = { "─", "│", "╭", "╮", "╰", "╯" };
static constexpr Border selected_border = { "═", "║", "╔", "╗", "╚", "╝" };

// curses box() is annoyingly for subwindows only
struct Box {
	int left, top, width, height;
	bool selected;

	int right() const { return left + width; }
	int bottom() const { return top + height; }

	void draw(WINDOW *window, char background) const {
		const Border& border = this->selected ? selected_border : normal_border;

		mvwaddstr(window, this->top, this->left, border.upper_left);
		for (int x = this->left + 1; x < this->right() - 1; x++)
			mvwaddstr(window, this->top, x, border.horizontal);
		mvwaddstr(window, this->top, this->right()-1, border.upper_right);

		for (int y=this->top+1; y < this->bottom() - 1; y++) {
			mvwaddstr(window, y, this->left, border.vertical);
			for (int x = this->left + 1; x < this->right() - 1; x++)
				mvwaddch(window, y, x, background);
			mvwaddstr(window, y, this->right() - 1, border.vertical);
		}

		mvwaddstr(window, this->bottom() - 1, this->left, border.lower_left);
		for (int x = this->left + 1; x < this->right() - 1; x++)
			mvwaddstr(window, this->bottom() - 1, x, border.horizontal);
		mvwaddstr(window, this->bottom() - 1, this->right() - 1, border.lower_right);
	}
};

// newwin() doesn't work because partially erasing borders is surprisingly tricky
// partial erasing is needed for cards that are on top of cards
// since we can't use subwindow borders, they're not very helpful
static void draw_card(WINDOW *window, const Card *card, int left, int top, bool selected, bool color)
{
	Box box = { left, top, CARD_WIDTH, CARD_HEIGHT, selected };
	if (card || selected) {
		char background = (card && !card->visible) ? '?' : ' ';
		box.draw(window, background);
	}

	if (card && card->visible) {
		int attributes = COLOR_PAIR(card->suit.color().color_pair_number());
		if (color)
			wattron(window, attributes);

		std::string number_string = card->number_string();
		std::string suit_string = card->suit.string();
		mvaddstr(top+1, left+1, number_string.c_str());
		mvaddstr(box.bottom()-2, box.right() - 1 - number_string.length(), number_string.c_str());
		mvaddstr(top+1, box.right()-2, suit_string.c_str());
		mvaddstr(box.bottom()-2, left+1, suit_string.c_str());

		if (color)
			wattroff(window, attributes);
	}
}

// unlike a simple for loop, handles overflow
static void draw_card_stack(WINDOW *window, const Card *bottom_card, int left, int top, const Card *first_selected, bool color)
{
	if (!bottom_card)
		return;

	int terminal_width, terminal_height;
	getmaxyx(window, terminal_height, terminal_width);
	(void)terminal_width;   // suppress warning about unused var

	// the text (number and suit) of bottom_card is at top+1
	// let's figure out where it is for the topmost card
	int top_text_y = top+1;
	int total_card_count = 1;
	for (Card *card = bottom_card->next /* bottom_card already counted */ ; card; card = card->next) {
		top_text_y += Y_OFFSET_BIG;
		total_card_count++;
	}

	// we can make all cards visible by displaying some cards with a smaller offset
	// we'lower_left display visible_card_count cards with the bigger offset
	int visible_card_count = total_card_count;
	while (top_text_y >= terminal_height) {
		top_text_y -= Y_OFFSET_BIG;
		top_text_y += Y_OFFSET_SMALL;
		visible_card_count--;
	}

	// to give some extra room that wouldn't be really necessary, but is nicer
	// without the if, cards get stacked even when there's enough room
	if (visible_card_count != total_card_count)
		visible_card_count--;

	// let's draw the cards
	bool selected = false;
	int y = top;
	for (const Card *card = bottom_card; card; card = card->next) {
		if (card == first_selected)
			selected = true;
		draw_card(window, card, left, y, selected, color);
		y += (--visible_card_count > 0) ? Y_OFFSET_BIG : Y_OFFSET_SMALL;
	}
}

// https://github.com/Akuli/curses-klondike/issues/2
enum class DiscardHide { HIDE_ALL, SHOW_LAST_ONLY, SHOW_ALL };

// TODO: this function is quite long, break it up
static void draw_game(WINDOW *window, const Klondike& klon, const Selection& sel, bool moving, bool color, DiscardHide discard_hide, int discard_x_offset)
{
	werase(window);

	int terminal_width, terminal_height;
	getmaxyx(window, terminal_height, terminal_width);

	// drawing just one card is enough for this
	draw_card(window, klon.stock, ui_x(0, terminal_width), ui_y(0), sel.place == CardPlace::stock(), color);

	int cards_shown_in_discard = klon.discardshow;
	if (sel.place == CardPlace::discard() && moving)
		cards_shown_in_discard++;

	int x = ui_x(1, terminal_width);
	for (Card *card = cardlist::top_n(klon.discard, cards_shown_in_discard); card; (card = card->next), (x += discard_x_offset)) {
		Card temp_card = *card;

		assert(temp_card.visible);
		if (discard_hide == DiscardHide::HIDE_ALL)
			temp_card.visible = false;
		if (discard_hide == DiscardHide::SHOW_LAST_ONLY)
			temp_card.visible = !card->next;

		draw_card(window, &temp_card, x, ui_y(0), sel.place == CardPlace::discard() && !card->next, color);
	}

	if (!klon.discard)   // nothing was drawn, but if the discard is selected, at least draw that
		draw_card(window, nullptr, ui_x(1, terminal_width), ui_y(0), sel.place == CardPlace::discard(), color);

	// foundations are similar to discard
	for (int i=0; i < 4; i++)
		draw_card(window, cardlist::top(klon.foundations[i]), ui_x(3+i, terminal_width), ui_y(0), sel.place == CardPlace::foundation(i), color);

	// now the tableau... here we go
	for (int x=0; x < 7; x++) {
		if (!klon.tableau[x]) {
			// draw a border if the tableau item is selected
			draw_card(window, nullptr, ui_x(x, terminal_width), ui_y(1), sel.place == CardPlace::tableau(x), color);
			continue;
		}

		int y = ui_y(1);
		for (Card *card = klon.tableau[x]; card; card = card->next) {
			if (card->visible) {
				draw_card_stack(window, card, ui_x(x, terminal_width), y, sel.card, color);
				break;
			}

			draw_card(window, card, ui_x(x, terminal_width), y, false, color);
			y += Y_OFFSET_SMALL;
		}
	}

	if (klon.win()) {
		std::string msg = "you win :)";
		mvwaddstr(window, terminal_height/2, (terminal_width - msg.length())/2, msg.c_str());
	}
}

static DiscardHide decide_what_to_hide(const SelectionOrMove& selmv, bool command_line_option)
{
	if (!command_line_option)
		return DiscardHide::SHOW_ALL;

	if (!selmv.ismove)
		return DiscardHide::SHOW_LAST_ONLY;

	if (selmv.move.src == CardPlace::discard() && selmv.move.dest == CardPlace::discard())
		return DiscardHide::SHOW_LAST_ONLY;
	if (selmv.move.src == CardPlace::discard())
		return DiscardHide::HIDE_ALL;
	return DiscardHide::SHOW_LAST_ONLY;
}

void ui_drawklon(WINDOW *window, const Klondike& klon, const SelectionOrMove& selmv, bool color, bool discardhide)
{
	DiscardHide discard_hide = decide_what_to_hide(selmv, discardhide);
	int discard_x_offset = discardhide ? 1 : X_OFFSET;

	if (selmv.ismove) {
		std::array<Card, 13*4> temp_cards;
		Klondike temp_klon;
		Selection temp_sel = { klon.dup(temp_klon, selmv.move.card, temp_cards), selmv.move.dest };
		temp_klon.move(temp_sel.card, temp_sel.place, true);
		draw_game(window, temp_klon, temp_sel, true, color, discard_hide, discard_x_offset);
	} else
		draw_game(window, klon, selmv.sel, false, color, discard_hide, discard_x_offset);
}
