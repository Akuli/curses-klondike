#include "selmv.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "card.h"
#include "klon.h"

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
	assert(0);
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

static struct Card *get_visible_top_card(struct Klon kln, KlonCardPlace plc)
{
	if (KLON_IS_FOUNDATION(plc))
		return card_top(kln.foundations[KLON_FOUNDATION_NUM(plc)]);
	if (KLON_IS_TABLEAU(plc))
		return card_top(kln.tableau[KLON_TABLEAU_NUM(plc)]);
	if (plc == KLON_DISCARD)
		return card_top(kln.discard);
	return NULL;
}

void selmv_byplace(struct Klon kln, struct SelMv *selmv, KlonCardPlace plc)
{
	if (selmv->ismv) {
		selmv->mv.dst = plc;
	} else {
		selmv->sel.place = plc;
		selmv->sel.card = get_visible_top_card(kln, plc);
	}
}

// if tabfndonly, only allows moving to tableau or foundations
static bool change_x_left_right(int *x, enum SelDirection dir, bool tab, bool tabfndonly)
{
	do
		*x += (dir == SEL_LEFT) ? -1 : 1;
	while (0 <= *x && *x < 7 && !tab && !card_x_2_top_place(*x));

	if (tabfndonly && !tab && !KLON_IS_FOUNDATION(card_x_2_top_place(*x)))
		return false;
	return (0 <= *x && *x < 7);
}

bool sel_more(struct Klon kln, struct Sel *sel)
{
	if (!KLON_IS_TABLEAU(sel->place))
		return false;

	for (struct Card *crd = kln.tableau[KLON_TABLEAU_NUM(sel->place)]; crd && crd->next; crd = crd->next)
		if (sel->card == crd->next && crd->visible) {
			sel->card = crd;
			return true;
		}
	return false;
}

bool sel_less(struct Klon kln, struct Sel *sel)
{
	if (KLON_IS_TABLEAU(sel->place) && sel->card && sel->card->next) {
		sel->card = sel->card->next;
		return true;
	}
	return false;
}

void selmv_anothercard(struct Klon kln, struct SelMv *selmv, enum SelDirection dir)
{
	if (selmv->ismv)
		assert(selmv->mv.card);

	int x = place_2_card_x(selmv->ismv ? selmv->mv.dst : selmv->sel.place);
	bool tab = KLON_IS_TABLEAU(selmv->ismv ? selmv->mv.dst : selmv->sel.place);

	switch(dir) {
	case SEL_LEFT:
	case SEL_RIGHT:
		if (change_x_left_right(&x, dir, tab, selmv->ismv))
			selmv_byplace(kln, selmv, tab ? KLON_TABLEAU(x) : card_x_2_top_place(x));
		break;

	case SEL_UP:
		if (selmv->ismv) {
			// can only move from table to foundations, but multiple cards not even there
			if (tab && KLON_IS_FOUNDATION(card_x_2_top_place(x)) && !selmv->mv.card->next)
				selmv_byplace(kln, selmv, card_x_2_top_place(x));
		} else
			if (tab && card_x_2_top_place(x))
				selmv_byplace(kln, selmv, card_x_2_top_place(x));
		break;

	case SEL_DOWN:
		if (selmv->ismv || !tab)
			selmv_byplace(kln, selmv, KLON_TABLEAU(x));
		break;

	default:
		assert(0);
	}
}

void selmv_beginmv(struct SelMv *selmv)
{
	selmv->ismv = true;
	selmv->mv.card = selmv->sel.card;
	selmv->mv.src = selmv->mv.dst = selmv->sel.place;
}

void selmv_endmv(struct Klon *kln, struct SelMv *selmv)
{
	assert(selmv->ismv);
	assert(selmv->mv.card);
	if (klon_canmove(*kln, selmv->mv.card, selmv->mv.dst))
		klon_move(kln, selmv->mv.card, selmv->mv.dst, false);

	selmv->ismv = false;
	selmv_byplace(*kln, selmv, selmv->mv.dst);
}
