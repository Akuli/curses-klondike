#include "card.hpp"
#include "klon.hpp"
#include "selectmove.hpp"
#include <array>
#include <cassert>
#include <optional>
#include <stdexcept>

static int place_2_card_x(CardPlace plc)
{
	switch(plc.kind) {
	case CardPlace::TABLEAU:
		return plc.number;
	case CardPlace::FOUNDATION:
		return 3 + plc.number;
	case CardPlace::STOCK:
		return 0;
	case CardPlace::DISCARD:
		return 1;
	}
	throw std::logic_error("bad place kind");
}

static std::optional<CardPlace> card_x_2_top_place(int x)
{
	if (x == 0)
		return CardPlace::stock();
	if (x == 1)
		return CardPlace::discard();
	if (3 <= x && x < 7)
		return CardPlace::foundation(x-3);
	return std::nullopt;
}

static Card *get_visible_top_card(Klondike klon, CardPlace plc)
{
	switch(plc.kind) {
		case CardPlace::FOUNDATION:
			return cardlist::top(klon.foundations[plc.number]);
		case CardPlace::TABLEAU:
			return cardlist::top(klon.tableau[plc.number]);
		case CardPlace::DISCARD:
			return cardlist::top(klon.discard);
		case CardPlace::STOCK:
			return nullptr;
	}
	throw std::logic_error("bad place kind");
}

void SelectionOrMove::select_top_card_at_place(const Klondike& klon, CardPlace plc)
{
	if (this->ismove)
		this->move.dest = plc;
	else
		this->sel = Selection{ get_visible_top_card(klon, plc), plc };
}

// if tabfndonly, only allows moving to tableau or foundations
static bool change_x_left_right(int *x, SelDirection dir, bool tab, bool tabfndonly)
{
	do
		*x += (dir == SelDirection::LEFT) ? -1 : 1;
	while (0 <= *x && *x < 7 && !tab && !card_x_2_top_place(*x));

	if (tabfndonly && !tab && card_x_2_top_place(*x).value().kind != CardPlace::FOUNDATION)
		return false;
	return (0 <= *x && *x < 7);
}

bool Selection::more(const Klondike& klon)
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

bool Selection::less(const Klondike& klon)
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

	int x = place_2_card_x(this->ismove ? this->move.dest : this->sel.place);
	std::optional<CardPlace> topplace = card_x_2_top_place(x);
	bool tab = (this->ismove ? this->move.dest : this->sel.place).kind == CardPlace::TABLEAU;

	switch(dir) {
	case SelDirection::LEFT:
	case SelDirection::RIGHT:
		if (change_x_left_right(&x, dir, tab, this->ismove))
			this->select_top_card_at_place(klon, tab ? CardPlace::tableau(x) : card_x_2_top_place(x).value());
		break;

	case SelDirection::UP:
		if (this->ismove) {
			// can only move from table to foundations, but multiple cards not even there
			if (tab && topplace && topplace.value().kind == CardPlace::FOUNDATION && !this->move.card->next)
				this->select_top_card_at_place(klon, topplace.value());
		} else {
			if (tab && topplace)
				this->select_top_card_at_place(klon, topplace.value());
		}
		break;

	case SelDirection::DOWN:
		if (this->ismove || !tab)
			this->select_top_card_at_place(klon, CardPlace::tableau(x));
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
	this->select_top_card_at_place(klon, this->move.dest);
}
