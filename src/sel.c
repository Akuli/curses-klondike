#include "sel.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "card.h"
#include "sol.h"
#include "ui.h"

static int place_2_card_x(SolCardPlace plc)
{
	if (SOL_IS_TABLEAU(plc))
		return SOL_TABLEAU_NUM(plc);
	if (SOL_IS_FOUNDATION(plc))
		return 3 + SOL_FOUNDATION_NUM(plc);
	if (plc == SOL_STOCK)
		return 0;
	if (plc == SOL_DISCARD)
		return 1;
	assert(0);
}

static SolCardPlace card_x_2_top_place(int x)
{
	if (x == 0)
		return SOL_STOCK;
	if (x == 1)
		return SOL_DISCARD;
	if (x == 2)
		return 0;

	assert(SOL_IS_FOUNDATION(SOL_FOUNDATION(x-3)));
	return SOL_FOUNDATION(x-3);
}

static struct Card *get_visible_top_card(struct Sol sol, SolCardPlace plc)
{
	if (SOL_IS_FOUNDATION(plc))
		return card_top(sol.foundations[SOL_FOUNDATION_NUM(plc)]);
	if (SOL_IS_TABLEAU(plc))
		return card_top(sol.tableau[SOL_TABLEAU_NUM(plc)]);
	if (plc == SOL_DISCARD)
		return card_top(sol.discard);
	return NULL;
}

static void set_place(struct Sol sol, struct UiSelection *sel, SolCardPlace plc)
{
	sel->place = plc;
	sel->card = get_visible_top_card(sol, plc);
}

// if tabfndonly, only allows moving to tableau or foundations
static bool change_x_left_right(int *x, enum SelDirection dir, bool tab, bool tabfndonly)
{
	do
		*x += (dir == SEL_LEFT) ? -1 : 1;
	while (0 <= *x && *x < 7 && !tab && !card_x_2_top_place(*x));

	if (tabfndonly && !tab && !SOL_IS_FOUNDATION(card_x_2_top_place(*x)))
		return false;
	return (0 <= *x && *x < 7);
}

void sel_anothercard(struct Sol sol, struct UiSelection *sel, enum SelDirection dir)
{
	int x = place_2_card_x(sel->place);
	bool tab = SOL_IS_TABLEAU(sel->place);

	switch(dir) {
	case SEL_LEFT:
	case SEL_RIGHT:
		if (change_x_left_right(&x, dir, tab, false))
			set_place(sol, sel, tab ? SOL_TABLEAU(x) : card_x_2_top_place(x));
		break;

	case SEL_UP:
		if (!tab)
			break;

		// can select more cards?
		for (struct Card *crd = sol.tableau[x]; crd && crd->next; crd = crd->next)
			if (sel->card == crd->next && crd->visible) {
				sel->card = crd;
				return;
			}

		// if not, move selection to top row if possible
		if (card_x_2_top_place(x))
			set_place(sol, sel, card_x_2_top_place(x));
		break;

	case SEL_DOWN:
		if (tab && sel->card && sel->card->next)
			sel->card = sel->card->next;
		if (!tab)
			set_place(sol, sel, SOL_TABLEAU(x));
		break;

	default:
		assert(0);
	}
}

void sel_anothercardmv(struct Sol sol, struct UiSelection sel, enum SelDirection dir, SolCardPlace *mv)
{
	assert(sel.card);
	int x = place_2_card_x(*mv);
	bool tab = SOL_IS_TABLEAU(*mv);

	switch(dir) {
	case SEL_LEFT:
	case SEL_RIGHT:
		if (change_x_left_right(&x, dir, tab, true))
			*mv = tab ? SOL_TABLEAU(x) : card_x_2_top_place(x);
		break;

	case SEL_UP:
		if (tab) {
			// can only move to foundations, but multiple cards not even there
			if (SOL_IS_FOUNDATION(card_x_2_top_place(x)) && !sel.card->next)
				*mv = card_x_2_top_place(x);
		} else
			*mv = SOL_TABLEAU(x);
		break;

	case SEL_DOWN:
		*mv = SOL_TABLEAU(x);
		break;

	default:
		assert(0);
	}
}

void sel_endmv(struct Sol *sol, struct UiSelection *sel, SolCardPlace mv)
{
	assert(sel->card);
	if (sol_canmove(*sol, sel->card, mv))
		sol_move(sol, sel->card, mv);
	set_place(*sol, sel, mv);
}
