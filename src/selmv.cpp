#include "selmv.hpp"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "card.hpp"
#include "klon.hpp"

static int place_2_card_x(KlonCardPlace plc)
{
	if (KLON_IS_TABLEAU(plc))
		return KLON_TABLEAU_NUM(plc);
	if (KLON_IS_FOUNDATION(plc))
		return 3 + KLON_FOUNDATION_NUM(plc);
	if (plc == KLON_STOCK)
		return 0;
	if (plc == KLON_DISCARD)
		return 1;
	throw std::logic_error("bad card place");
}

static KlonCardPlace card_x_2_top_place(int x)
{
	if (x == 0)
		return KLON_STOCK;
	if (x == 1)
		return KLON_DISCARD;
	if (KLON_IS_FOUNDATION(KLON_FOUNDATION(x-3)))
		return KLON_FOUNDATION(x-3);
	return 0;
}

static Card *get_visible_top_card(Klon kln, KlonCardPlace plc)
{
	if (KLON_IS_FOUNDATION(plc))
		return card_top(kln.foundations[KLON_FOUNDATION_NUM(plc)]);
	if (KLON_IS_TABLEAU(plc))
		return card_top(kln.tableau[KLON_TABLEAU_NUM(plc)]);
	if (plc == KLON_DISCARD)
		return card_top(kln.discard);
	return NULL;
}

void selmv_byplace(Klon kln, SelMv *selmv, KlonCardPlace plc)
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

	if (tabfndonly && !tab && !KLON_IS_FOUNDATION(card_x_2_top_place(*x)))
		return false;
	return (0 <= *x && *x < 7);
}

bool sel_more(Klon kln, Sel *sel)
{
	if (!KLON_IS_TABLEAU(sel->place))
		return false;

	for (Card *crd = kln.tableau[KLON_TABLEAU_NUM(sel->place)]; crd && crd->next; crd = crd->next)
		if (sel->card == crd->next && crd->visible) {
			sel->card = crd;
			return true;
		}
	return false;
}

bool sel_less(Klon kln, Sel *sel)
{
	if (KLON_IS_TABLEAU(sel->place) && sel->card && sel->card->next) {
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
	bool tab = KLON_IS_TABLEAU(selmv->ismv ? selmv->mv.dst : selmv->sel.place);

	switch(dir) {
	case SelDirection::LEFT:
	case SelDirection::RIGHT:
		if (change_x_left_right(&x, dir, tab, selmv->ismv))
			selmv_byplace(kln, selmv, tab ? KLON_TABLEAU(x) : card_x_2_top_place(x));
		break;

	case SelDirection::UP:
		if (selmv->ismv) {
			// can only move from table to foundations, but multiple cards not even there
			if (tab && KLON_IS_FOUNDATION(card_x_2_top_place(x)) && !selmv->mv.card->next)
				selmv_byplace(kln, selmv, card_x_2_top_place(x));
		} else
			if (tab && card_x_2_top_place(x))
				selmv_byplace(kln, selmv, card_x_2_top_place(x));
		break;

	case SelDirection::DOWN:
		if (selmv->ismv || !tab)
			selmv_byplace(kln, selmv, KLON_TABLEAU(x));
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
