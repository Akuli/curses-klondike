#include "klon.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.h"
#include "misc.h"

void klon_init(struct Klon *kln, struct Card *list)
{
	for (int i=0; i < 7; i++) {
		kln->tableau[i] = NULL;
		for (int j=0; j < i+1; j++)
			card_pushtop(&kln->tableau[i], card_popbot(&list));
		card_top(kln->tableau[i])->visible = true;
	}

	kln->stock = list;
	kln->discard = NULL;
	for (int i=0; i < 4; i++)
		kln->foundations[i] = NULL;
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

void klon_debug(struct Klon kln)
{
	int total = 0;

	printf("stock:");
	total += print_cards(kln.stock);

	printf("discard:");
	total += print_cards(kln.discard);

	for (int i=0; i < 4; i++) {
		printf("foundation %d:", i);
		total += print_cards(kln.foundations[i]);
	}

	for (int i=0; i < 7; i++) {
		printf("tableau %d:", i);
		total += print_cards(kln.tableau[i]);
	}

	printf("total: %d cards\n", total);
}

void klon_free(struct Klon kln)
{
	card_free(kln.stock);
	card_free(kln.discard);
	for (int i=0; i < 4; i++)
		card_free(kln.foundations[i]);
	for (int i=0; i < 7; i++)
		card_free(kln.tableau[i]);
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

struct Card *klon_dup(struct Klon src, struct Klon *dst, struct Card *srccrd)
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

static bool card_in_some_tableau(struct Klon kln, struct Card *crd)
{
	for (int i=0; i < 7; i++)
		for (struct Card *tabcrd = kln.tableau[i]; tabcrd; tabcrd = tabcrd->next)
			if (tabcrd == crd)
				return true;
	return false;
}

bool klon_canmove(struct Klon kln, struct Card *crd, KlonCardPlace dst)
{
	// taking cards stock to discard is handled by klon_stactodiscard() and not allowed here
	if (crd->next) {
		// the only way how a stack of multiple cards is allowed to move is tableau -> tableau
		if (!(card_in_some_tableau(kln, crd) && KLON_IS_TABLEAU(dst)))
			return false;
		goto tableau;
	}

	if (crd->next || dst == KLON_STOCK || dst == KLON_DISCARD || !crd->visible)
		return false;

	if (KLON_IS_FOUNDATION(dst)) {
		struct Card *fnd = kln.foundations[KLON_FOUNDATION_NUM(dst)];
		if (!fnd)
			return (crd->num == 1);

		fnd = card_top(fnd);
		return (crd->suit == fnd->suit && crd->num == fnd->num + 1);
	}

	tableau:
	if (KLON_IS_TABLEAU(dst)) {
		struct Card *tab = kln.tableau[KLON_TABLEAU_NUM(dst)];
		if (!tab)
			return (crd->num == 13);

		tab = card_top(tab);
		return (SUIT_COLOR(crd->suit) != SUIT_COLOR(tab->suit) && crd->num == tab->num - 1);
	}

	assert(0);
}

// a double-linked list would make this easier but many other things harder
struct Card *klon_detachcard(struct Klon *kln, struct Card *crd)
{
	struct Card **look4[2+4+7];
	struct Card ***look4ptr = look4;
	*look4ptr++ = &kln->stock;
	*look4ptr++ = &kln->discard;
	for (int i=0; i < 4; i++)
		*look4ptr++ = &kln->foundations[i];
	for (int i=0; i < 7; i++)
		*look4ptr++ = &kln->tableau[i];

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

static void do_move(struct Klon *kln, struct Card *crd, KlonCardPlace dst, bool raw)
{
	if (!raw)
		assert(klon_canmove(*kln, crd, dst));

	struct Card *prv = klon_detachcard(kln, crd);

	// prv:
	//  * is NULL, if crd was the bottommost card
	//  * has ->next==true, if moving only some of many visible cards on top of each other
	//  * has ->next==false, if crd was the bottommost VISIBLE card in the tableau list
	if (prv && !raw)
		prv->visible = true;

	struct Card **dstp;
	if (dst == KLON_STOCK)
		dstp = &kln->stock;
	else if (dst == KLON_DISCARD)
		dstp = &kln->discard;
	else if (KLON_IS_FOUNDATION(dst))
		dstp = &kln->foundations[KLON_FOUNDATION_NUM(dst)];
	else if (KLON_IS_TABLEAU(dst))
		dstp = &kln->tableau[KLON_TABLEAU_NUM(dst)];
	else
		assert(0);

	card_pushtop(dstp, crd);
}

void klon_move(struct Klon *kln, struct Card *crd, KlonCardPlace dst) { do_move(kln, crd, dst, false); }
void klon_rawmove(struct Klon *kln, struct Card *crd, KlonCardPlace dst) { do_move(kln, crd, dst, true); }

void klon_stock2discard(struct Klon *kln)
{
	if (!kln->stock) {
		for (struct Card *crd = kln->discard; crd; crd = crd->next) {
			assert(crd->visible);
			crd->visible = false;
		}

		kln->stock = kln->discard;   // may be NULL when all stock cards have been used
		kln->discard = NULL;
		return;
	}

	// the card on top of the stock must be visible, but other stock cards aren't
	struct Card *pop = card_popbot(&kln->stock);
	assert(!pop->visible);
	pop->visible = true;
	card_pushtop(&kln->discard, pop);
}

void sel_2foundation(struct Klon *kln, struct Card *crd)
{
	for (int i=0; i < 4; i++)
		if (klon_canmove(*kln, crd, KLON_FOUNDATION(i))) {
			klon_move(kln, crd, KLON_FOUNDATION(i));
			break;
		}
}

bool klon_win(struct Klon kln)
{
	for (int i=0; i < 4; i++) {
		int n = 0;
		for (struct Card *crd = kln.foundations[i]; crd; crd = crd->next)
			n++;

		assert(n <= 13);
		if (n < 13)
			return false;
	}

	return true;
}
