#include "sel.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "card.h"
#include "klon.h"
#include "ui.h"

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

void sel_byplace(struct Klon kln, struct Sel *sel, KlonCardPlace plc)
{
	sel->place = plc;
	sel->card = get_visible_top_card(kln, plc);
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

void sel_anothercard(struct Klon kln, struct Sel *sel, enum SelDirection dir)
{
	int x = place_2_card_x(sel->place);
	bool tab = KLON_IS_TABLEAU(sel->place);

	switch(dir) {
	case SEL_LEFT:
	case SEL_RIGHT:
		if (change_x_left_right(&x, dir, tab, false))
			sel_byplace(kln, sel, tab ? KLON_TABLEAU(x) : card_x_2_top_place(x));
		break;

	case SEL_UP:
		if (!tab)
			break;

		if (card_x_2_top_place(x))
			sel_byplace(kln, sel, card_x_2_top_place(x));
		break;

	case SEL_DOWN:
		if (!tab)
			sel_byplace(kln, sel, KLON_TABLEAU(x));
		break;

	default:
		assert(0);
	}
}

void sel_anothercardmv(struct Klon kln, struct Sel sel, enum SelDirection dir, KlonCardPlace *mv)
{
	assert(sel.card);
	int x = place_2_card_x(*mv);
	bool tab = KLON_IS_TABLEAU(*mv);

	switch(dir) {
	case SEL_LEFT:
	case SEL_RIGHT:
		if (change_x_left_right(&x, dir, tab, true))
			*mv = tab ? KLON_TABLEAU(x) : card_x_2_top_place(x);
		break;

	case SEL_UP:
		if (tab) {
			// can only move to foundations, but multiple cards not even there
			if (KLON_IS_FOUNDATION(card_x_2_top_place(x)) && !sel.card->next)
				*mv = card_x_2_top_place(x);
		} else
			*mv = KLON_TABLEAU(x);
		break;

	case SEL_DOWN:
		*mv = KLON_TABLEAU(x);
		break;

	default:
		assert(0);
	}
}

void sel_endmv(struct Klon *kln, struct Sel *sel, KlonCardPlace mv)
{
	assert(sel->card);
	if (klon_canmove(*kln, sel->card, mv))
		klon_move(kln, sel->card, mv, false);
	sel_byplace(*kln, sel, mv);
}
