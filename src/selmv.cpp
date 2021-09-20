#include "selmv.hpp"
#include <optional>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "card.hpp"
#include "klon.hpp"

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
		return CardPlace(CardPlace::STOCK);
	if (x == 1)
		return CardPlace(CardPlace::DISCARD);
	if (3 <= x && x < 7)
		return CardPlace(CardPlace::FOUNDATION, x-3);
	return std::nullopt;
}

static Card *get_visible_top_card(Klon kln, CardPlace plc)
{
	switch(plc.kind) {
		case CardPlace::FOUNDATION:
			return card_top(kln.foundations[plc.num]);
		case CardPlace::TABLEAU:
			return card_top(kln.tableau[plc.num]);
		case CardPlace::DISCARD:
			return card_top(kln.discard);
		case CardPlace::STOCK:
			return NULL;
	}
	throw std::logic_error("bad place kind");
}

void selmv_byplace(Klon kln, SelMv *selmv, CardPlace plc)
{
	if (selmv->ismv) {
		selmv->mv.dst = plc;
	} else {
		selmv->sel.place = plc;
		selmv->sel.card = get_visible_top_card(kln, plc);
	}
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

bool sel_more(Klon kln, Sel *sel)
{
	if (sel->place.kind != CardPlace::TABLEAU)
		return false;

	for (Card *crd = kln.tableau[sel->place.num]; crd && crd->next; crd = crd->next)
		if (sel->card == crd->next && crd->visible) {
			sel->card = crd;
			return true;
		}
	return false;
}

bool sel_less(Klon kln, Sel *sel)
{
	if (sel->place.kind == CardPlace::TABLEAU && sel->card && sel->card->next) {
		sel->card = sel->card->next;
		return true;
	}
	return false;
}

void selmv_anothercard(Klon kln, SelMv *selmv, SelDirection dir)
{
	if (selmv->ismv)
		assert(selmv->mv.card);

	int x = place_2_card_x(selmv->ismv ? selmv->mv.dst : selmv->sel.place);
	std::optional<CardPlace> topplace = card_x_2_top_place(x);
	bool tab = (selmv->ismv ? selmv->mv.dst : selmv->sel.place).kind == CardPlace::TABLEAU;

	switch(dir) {
	case SelDirection::LEFT:
	case SelDirection::RIGHT:
		if (change_x_left_right(&x, dir, tab, selmv->ismv))
			selmv_byplace(kln, selmv, tab ? CardPlace(CardPlace::TABLEAU, x) : card_x_2_top_place(x).value());
		break;

	case SelDirection::UP:
		if (selmv->ismv) {
			// can only move from table to foundations, but multiple cards not even there
			if (tab && topplace && topplace.value().kind == CardPlace::FOUNDATION && !selmv->mv.card->next)
				selmv_byplace(kln, selmv, topplace.value());
		} else
			if (tab && topplace)
				selmv_byplace(kln, selmv, topplace.value());
		break;

	case SelDirection::DOWN:
		if (selmv->ismv || !tab)
			selmv_byplace(kln, selmv, CardPlace(CardPlace::TABLEAU, x));
		break;
	}
}

void selmv_beginmv(SelMv *selmv)
{
	selmv->ismv = true;
	selmv->mv.card = selmv->sel.card;
	selmv->mv.src = selmv->mv.dst = selmv->sel.place;
}

void selmv_endmv(Klon *kln, SelMv *selmv)
{
	assert(selmv->ismv);
	assert(selmv->mv.card);
	if (kln->canmove(selmv->mv.card, selmv->mv.dst))
		kln->move(selmv->mv.card, selmv->mv.dst, false);

	selmv->ismv = false;
	selmv_byplace(*kln, selmv, selmv->mv.dst);
}
