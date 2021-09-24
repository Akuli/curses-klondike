#include "card.hpp"
#include "klon.hpp"
#include "selectmove.hpp"
#include <array>
#include <cassert>
#include <optional>
#include <stdexcept>

static int place_to_card_x(CardPlace place)
{
	switch(place.kind) {
	case CardPlace::TABLEAU:
		return place.number;
	case CardPlace::FOUNDATION:
		return 3 + place.number;
	case CardPlace::STOCK:
		return 0;
	case CardPlace::DISCARD:
		return 1;
	}
	throw std::logic_error("bad place kind");
}

static std::optional<CardPlace> card_x_to_top_place(int x)
{
	if (x == 0)
		return CardPlace::stock();
	if (x == 1)
		return CardPlace::discard();
	if (3 <= x && x < 7)
		return CardPlace::foundation(x-3);
	return std::nullopt;
}

static Card *get_visible_top_card(Klondike klon, CardPlace place)
{
	switch(place.kind) {
		case CardPlace::FOUNDATION:
			return cardlist::top(klon.foundations[place.number]);
		case CardPlace::TABLEAU:
			return cardlist::top(klon.tableau[place.number]);
		case CardPlace::DISCARD:
			return cardlist::top(klon.discard);
		case CardPlace::STOCK:
			return nullptr;
	}
	throw std::logic_error("bad place kind");
}

void SelectionOrMove::select_top_card_or_move_to(const Klondike& klon, CardPlace place)
{
	if (this->ismove)
		this->move.dest = place;
	else
		this->sel = Selection{ get_visible_top_card(klon, place), place };
}

bool Selection::select_more(const Klondike& klon)
{
	if (this->place.kind != CardPlace::TABLEAU)
		return false;

	for (Card *card = klon.tableau[this->place.number]; card && card->next; card = card->next)
		if (this->card == card->next && card->visible) {
			this->card = card;
			return true;
		}
	return false;
}

bool Selection::select_less(const Klondike& klon)
{
	if (this->place.kind == CardPlace::TABLEAU && this->card && this->card->next) {
		this->card = this->card->next;
		return true;
	}
	return false;
}

void SelectionOrMove::select_another_card(const Klondike& klon, SelDirection dir)
{
	if (this->ismove)
		assert(this->move.card);

	int x = place_to_card_x(this->ismove ? this->move.dest : this->sel.place);
	bool is_bottom_row = (this->ismove ? this->move.dest : this->sel.place).kind == CardPlace::TABLEAU;

	switch(dir) {
	case SelDirection::LEFT:
	case SelDirection::RIGHT:
	{
		int dx = (dir == SelDirection::LEFT) ? -1 : 1;
		x += dx;
		if (x == 2 && !is_bottom_row)   // between discard and tableau
			x += dx;

		if (0 <= x && x < 7) {
			std::optional<CardPlace> new_place = is_bottom_row ? CardPlace::tableau(x) : card_x_to_top_place(x);
			assert(new_place);
			if (!this->ismove || is_bottom_row || new_place->kind == CardPlace::FOUNDATION)
				this->select_top_card_or_move_to(klon, new_place.value());
		}
		break;
	}

	case SelDirection::UP:
		if (is_bottom_row) {
			std::optional<CardPlace> top_place = card_x_to_top_place(x);

			// can only move from table to foundations, but multiple cards not even there
			if (top_place && (!this->ismove || (top_place->kind == CardPlace::FOUNDATION && !this->move.card->next)))
				this->select_top_card_or_move_to(klon, top_place.value());
		}
		break;

	case SelDirection::DOWN:
		if (this->ismove || !is_bottom_row)
			this->select_top_card_or_move_to(klon, CardPlace::tableau(x));
		break;
	}
}

void SelectionOrMove::begin_move()
{
	this->ismove = true;
	this->move.card = this->sel.card;
	this->move.src = this->move.dest = this->sel.place;
}

void SelectionOrMove::end_move(Klondike& klon)
{
	assert(this->ismove);
	assert(this->move.card);
	if (klon.can_move(this->move.card, this->move.dest))
		klon.move(this->move.card, this->move.dest, false);

	this->ismove = false;
	this->select_top_card_or_move_to(klon, this->move.dest);
}
