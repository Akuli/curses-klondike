#include "card.hpp"
#include "klon.hpp"
#include "selmv.hpp"
#include <cassert>
#include <optional>
#include <stdexcept>

static int place_2_card_x(CardPlace plc)
{
	switch(plc.kind) {
	case CardPlace::TABLEAU:
		return plc.num;
	case CardPlace::FOUNDATION:
		return 3 + plc.num;
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

static Card *get_visible_top_card(Klon kln, CardPlace plc)
{
	switch(plc.kind) {
		case CardPlace::FOUNDATION:
			return cardlist::top(kln.foundations[plc.num]);
		case CardPlace::TABLEAU:
			return cardlist::top(kln.tableau[plc.num]);
		case CardPlace::DISCARD:
			return cardlist::top(kln.discard);
		case CardPlace::STOCK:
			return nullptr;
	}
	throw std::logic_error("bad place kind");
}

void SelMv::select_top_card_at_place(const Klon& kln, CardPlace plc)
{
	if (this->ismv)
		this->mv.dst = plc;
	else
		this->sel = Sel{ get_visible_top_card(kln, plc), plc };
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

bool Sel::more(const Klon& kln)
{
	if (this->place.kind != CardPlace::TABLEAU)
		return false;

	for (Card *crd = kln.tableau[this->place.num]; crd && crd->next; crd = crd->next)
		if (this->card == crd->next && crd->visible) {
			this->card = crd;
			return true;
		}
	return false;
}

bool Sel::less(const Klon& kln)
{
	if (this->place.kind == CardPlace::TABLEAU && this->card && this->card->next) {
		this->card = this->card->next;
		return true;
	}
	return false;
}

void SelMv::select_another_card(const Klon& kln, SelDirection dir)
{
	if (this->ismv)
		assert(this->mv.card);

	int x = place_2_card_x(this->ismv ? this->mv.dst : this->sel.place);
	std::optional<CardPlace> topplace = card_x_2_top_place(x);
	bool tab = (this->ismv ? this->mv.dst : this->sel.place).kind == CardPlace::TABLEAU;

	switch(dir) {
	case SelDirection::LEFT:
	case SelDirection::RIGHT:
		if (change_x_left_right(&x, dir, tab, this->ismv))
			this->select_top_card_at_place(kln, tab ? CardPlace::tableau(x) : card_x_2_top_place(x).value());
		break;

	case SelDirection::UP:
		if (this->ismv) {
			// can only move from table to foundations, but multiple cards not even there
			if (tab && topplace && topplace.value().kind == CardPlace::FOUNDATION && !this->mv.card->next)
				this->select_top_card_at_place(kln, topplace.value());
		} else {
			if (tab && topplace)
				this->select_top_card_at_place(kln, topplace.value());
		}
		break;

	case SelDirection::DOWN:
		if (this->ismv || !tab)
			this->select_top_card_at_place(kln, CardPlace::tableau(x));
		break;
	}
}

void SelMv::begin_move()
{
	this->ismv = true;
	this->mv.card = this->sel.card;
	this->mv.src = this->mv.dst = this->sel.place;
}

void SelMv::end_move(Klon& kln)
{
	assert(this->ismv);
	assert(this->mv.card);
	if (kln.canmove(this->mv.card, this->mv.dst))
		kln.move(this->mv.card, this->mv.dst, false);

	this->ismv = false;
	this->select_top_card_at_place(kln, this->mv.dst);
}
