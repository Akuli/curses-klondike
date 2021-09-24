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

// https://github.com/Akuli/curses-klondike/issues/2
enum class DiscardHide { HIDE_ALL, SHOW_LAST_ONLY, SHOW_ALL };

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

struct Drawer {
	WINDOW *window;
	const Klondike *klon;
	const Selection *sel;
	bool moving;
	bool color;
	DiscardHide discard_hide;
	int discard_x_offset;

	int terminal_width()  const { int w,h; getmaxyx(this->window, h, w); (void)h; return w; }
	int terminal_height() const { int w,h; getmaxyx(this->window, h, w); (void)w; return h; }

	// newwin() doesn't work because partially erasing borders is surprisingly tricky
	// partial erasing is needed for cards that are on top of cards
	// since we can't use subwindow borders, they're not very helpful
	void draw_card(const Card *card, int left, int top, bool selected) const
	{
		Box box = { left, top, CARD_WIDTH, CARD_HEIGHT, selected };
		if (card || selected) {
			char background = (card && !card->visible) ? '?' : ' ';
			box.draw(this->window, background);
		}

		if (card && card->visible) {
			int attributes = COLOR_PAIR(card->suit.color().color_pair_number());
			if (this->color)
				wattron(this->window, attributes);

			std::string number_string = card->number_string();
			std::string suit_string = card->suit.string();
			mvaddstr(top+1, left+1, number_string.c_str());
			mvaddstr(box.bottom()-2, box.right() - 1 - number_string.length(), number_string.c_str());
			mvaddstr(top+1, box.right()-2, suit_string.c_str());
			mvaddstr(box.bottom()-2, left+1, suit_string.c_str());

			if (this->color)
				wattroff(this->window, attributes);
		}
	}

	// unlike a simple for loop, handles overflow
	void draw_card_stack(const Card *bottom_card, int left, int top, const Card *first_selected) const
	{
		if (!bottom_card)
			return;

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
		while (top_text_y >= this->terminal_height()) {
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
			this->draw_card(card, left, y, selected);
			y += (--visible_card_count > 0) ? Y_OFFSET_BIG : Y_OFFSET_SMALL;
		}
	}

	void draw_discard() const
	{
		int show_count = this->klon->discardshow;
		if (this->sel->place == CardPlace::discard() && this->moving)
			show_count++;

		int x = ui_x(1, this->terminal_width());
		for (Card *card = cardlist::top_n(this->klon->discard, show_count); card; card = card->next)
		{
			Card temp = *card;
			assert(temp.visible);
			if (discard_hide == DiscardHide::HIDE_ALL)
				temp.visible = false;
			if (discard_hide == DiscardHide::SHOW_LAST_ONLY)
				temp.visible = !card->next;

			this->draw_card(&temp, x, ui_y(0), this->sel->place == CardPlace::discard() && !card->next);
			x += discard_x_offset;
		}

		if (!this->klon->discard)   // nothing was drawn, but if the discard is selected, at least draw that
			this->draw_card(nullptr, ui_x(1, this->terminal_width()), ui_y(0), this->sel->place == CardPlace::discard());
	}

	void draw_tableau() const
	{
		for (int x=0; x < 7; x++) {
			if (!this->klon->tableau[x]) {
				// draw a border if the tableau item is selected
				this->draw_card(nullptr, ui_x(x, this->terminal_width()), ui_y(1), this->sel->place == CardPlace::tableau(x));
				continue;
			}

			int y = ui_y(1);
			for (Card *card = this->klon->tableau[x]; card; card = card->next) {
				if (card->visible) {
					this->draw_card_stack(card, ui_x(x, this->terminal_width()), y, this->sel->card);
					break;
				}

				this->draw_card(card, ui_x(x, this->terminal_width()), y, false);
				y += Y_OFFSET_SMALL;
			}
		}
	}

	void draw_game()
	{
		werase(this->window);

		draw_card(this->klon->stock, ui_x(0, this->terminal_width()), ui_y(0), this->sel->place == CardPlace::stock());
		this->draw_discard();
		for (int i=0; i < 4; i++)
			draw_card(cardlist::top(this->klon->foundations[i]), ui_x(3+i, this->terminal_width()), ui_y(0), this->sel->place == CardPlace::foundation(i));
		this->draw_tableau();

		if (this->klon->win()) {
			std::string msg = "you win :)";
			mvwaddstr(this->window, this->terminal_height()/2, (this->terminal_width() - msg.length())/2, msg.c_str());
		}
	}
};

void ui_draw(WINDOW *window, const Klondike& klon, const SelectionOrMove& selmv, const Args& args)
{
	DiscardHide discard_hide = decide_what_to_hide(selmv, args.discardhide);
	int discard_x_offset = args.discardhide ? 1 : X_OFFSET;

	Drawer drawer = { window, &klon, &selmv.sel, true, args.color, discard_hide, discard_x_offset };

	if (selmv.ismove) {
		std::array<Card, 13*4> temp_cards;
		Klondike temp_klon;
		Selection temp_sel = { klon.dup(temp_klon, selmv.move.card, temp_cards), selmv.move.dest };
		temp_klon.move(temp_sel.card, temp_sel.place, true);

		drawer.klon = &temp_klon;
		drawer.sel = &temp_sel;
		drawer.draw_game();
	} else {
		drawer.draw_game();
	}
}
