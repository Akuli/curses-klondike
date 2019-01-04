#include "sol.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.h"
#include "misc.h"

void sol_init(struct Sol *sol, struct Card *list)
{
	for (int i=0; i < 7; i++) {
		sol->tableau[i] = NULL;
		for (int j=0; j < i+1; j++)
			card_pushtop(&sol->tableau[i], card_popbot(&list));
		card_top(sol->tableau[i])->visible = true;
	}

	sol->stock = list;
	sol->discard = NULL;
	for (int i=0; i < 4; i++)
		sol->foundations[i] = NULL;
}

static int print_cards(struct Card *list)
{
	int n = 0;
	for (; list; list = list->next) {
		char sbuf[CARD_SUITSTRMAX], nbuf[CARD_NUMSTRMAX];
		card_suitstr(*list, sbuf);
		card_numstr(*list, nbuf);
		printf(" %c%s%s", list->visible ? 'v' : '?', sbuf, nbuf);
		n++;
	}
	printf(" (%d cards)\n", n);
	return n;
}

void sol_debug(struct Sol sol)
{
	int total = 0;

	printf("stock:");
	total += print_cards(sol.stock);

	printf("discard:");
	total += print_cards(sol.discard);

	for (int i=0; i < 4; i++) {
		printf("foundation %d:", i);
		total += print_cards(sol.foundations[i]);
	}

	for (int i=0; i < 7; i++) {
		printf("tableau %d:", i);
		total += print_cards(sol.tableau[i]);
	}

	printf("total: %d cards\n", total);
}

void sol_free(struct Sol sol)
{
	card_free(sol.stock);
	card_free(sol.discard);
	for (int i=0; i < 4; i++)
		card_free(sol.foundations[i]);
	for (int i=0; i < 7; i++)
		card_free(sol.tableau[i]);
}

static void copy_cards(struct Card *src, struct Card **dst, struct Card *srccrd, struct Card **dstcrd)
{
	*dst = NULL;
	struct Card *top = NULL;

	for (; src; src = src->next) {
		struct Card *dup = malloc(sizeof(struct Card));
		if (!dup)
			fatal_error("malloc() failed");
		if (src == srccrd)
			*dstcrd = dup;

		memcpy(dup, src, sizeof(struct Card));
		dup->next = NULL;

		if (top)
			top->next = dup;
		else
			*dst = dup;
		top = dup;
	}
}

struct Card *sol_dup(struct Sol src, struct Sol *dst, struct Card *srccrd)
{
	struct Card *dstcrd = NULL;

	copy_cards(src.stock, &dst->stock, srccrd, &dstcrd);
	copy_cards(src.discard, &dst->discard, srccrd, &dstcrd);
	for (int i=0; i < 4; i++)
		copy_cards(src.foundations[i], &dst->foundations[i], srccrd, &dstcrd);
	for (int i=0; i < 7; i++)
		copy_cards(src.tableau[i], &dst->tableau[i], srccrd, &dstcrd);

	return dstcrd;
}

static bool card_in_some_tableau(struct Sol sol, struct Card *crd)
{
	for (int i=0; i < 7; i++)
		for (struct Card *tabcrd = sol.tableau[i]; tabcrd; tabcrd = tabcrd->next)
			if (tabcrd == crd)
				return true;
	return false;
}

bool sol_canmove(struct Sol sol, struct Card *crd, SolCardPlace dst)
{
	// taking cards stock to discard is handled by sol_stactodiscard() and not allowed here
	if (crd->next) {
		// the only way how a stack of multiple cards is allowed to move is tableau -> tableau
		if (!(card_in_some_tableau(sol, crd) && SOL_IS_TABLEAU(dst)))
			return false;
		goto tableau;
	}

	if (crd->next || dst == SOL_STOCK || dst == SOL_DISCARD || !crd->visible)
		return false;

	if (SOL_IS_FOUNDATION(dst)) {
		struct Card *fnd = sol.foundations[SOL_FOUNDATION_NUM(dst)];
		if (!fnd)
			return (crd->num == 1);

		fnd = card_top(fnd);
		return (crd->suit == fnd->suit && crd->num == fnd->num + 1);
	}

	tableau:
	if (SOL_IS_TABLEAU(dst)) {
		struct Card *tab = sol.tableau[SOL_TABLEAU_NUM(dst)];
		if (!tab)
			return (crd->num == 13);

		tab = card_top(tab);
		return (SUIT_COLOR(crd->suit) != SUIT_COLOR(tab->suit) && crd->num == tab->num - 1);
	}

	assert(0);
}

// a double-linked list would make this easier but many other things harder
struct Card *sol_detachcard(struct Sol *sol, struct Card *crd)
{
	struct Card **look4[2+4+7];
	struct Card ***look4ptr = look4;
	*look4ptr++ = &sol->stock;
	*look4ptr++ = &sol->discard;
	for (int i=0; i < 4; i++)
		*look4ptr++ = &sol->foundations[i];
	for (int i=0; i < 7; i++)
		*look4ptr++ = &sol->tableau[i];

	for (unsigned int i=0; i < sizeof(look4)/sizeof(look4[0]); i++) {
		// special case: no card has crd as ->next
		if (*look4[i] == crd) {
			*look4[i] = NULL;
			return NULL;
		}

		for (struct Card *prv = *look4[i]; prv && prv->next; prv = prv->next)
			if (prv->next == crd) {
				prv->next = NULL;
				return prv;
			}
	}

	assert(0);
}

static void do_move(struct Sol *sol, struct Card *crd, SolCardPlace dst, bool raw)
{
	if (!raw)
		assert(sol_canmove(*sol, crd, dst));

	struct Card *prv = sol_detachcard(sol, crd);

	// prv:
	//  * is NULL, if crd was the bottommost card
	//  * has ->next==true, if moving only some of many visible cards on top of each other
	//  * has ->next==false, if crd was the bottommost VISIBLE card in the tableau list
	if (prv && !raw)
		prv->visible = true;

	struct Card **dstp;
	if (dst == SOL_STOCK)
		dstp = &sol->stock;
	else if (dst == SOL_DISCARD)
		dstp = &sol->discard;
	else if (SOL_IS_FOUNDATION(dst))
		dstp = &sol->foundations[SOL_FOUNDATION_NUM(dst)];
	else if (SOL_IS_TABLEAU(dst))
		dstp = &sol->tableau[SOL_TABLEAU_NUM(dst)];
	else
		assert(0);

	card_pushtop(dstp, crd);
}

void sol_move(struct Sol *sol, struct Card *crd, SolCardPlace dst) { do_move(sol, crd, dst, false); }
void sol_rawmove(struct Sol *sol, struct Card *crd, SolCardPlace dst) { do_move(sol, crd, dst, true); }

void sol_stock2discard(struct Sol *sol)
{
	if (!sol->stock) {
		for (struct Card *crd = sol->discard; crd; crd = crd->next) {
			assert(crd->visible);
			crd->visible = false;
		}

		sol->stock = sol->discard;   // may be NULL when all stock cards have been used
		sol->discard = NULL;
		return;
	}

	// the card on top of the stock must be visible, but other stock cards aren't
	struct Card *pop = card_popbot(&sol->stock);
	assert(!pop->visible);
	pop->visible = true;
	card_pushtop(&sol->discard, pop);
}

void sel_2foundation(struct Sol *sol, struct Card *crd)
{
	for (int i=0; i < 4; i++)
		if (sol_canmove(*sol, crd, SOL_FOUNDATION(i))) {
			sol_move(sol, crd, SOL_FOUNDATION(i));
			break;
		}
}

bool sol_win(struct Sol sol)
{
	for (int i=0; i < 4; i++) {
		int n = 0;
		for (struct Card *crd = sol.foundations[i]; crd; crd = crd->next)
			n++;

		assert(n <= 13);
		if (n < 13)
			return false;
	}

	return true;
}
