#include "klon.hpp"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.hpp"
#include "misc.hpp"

void Klon::init()
{
	Card *list = card_init_list(this->allcards);
	for (int i=0; i < 7; i++) {
		this->tableau[i] = nullptr;
		for (int j=0; j < i+1; j++)
			card_pushtop(&this->tableau[i], card_popbot(&list));
		card_top(this->tableau[i])->visible = true;
	}

	this->stock = list;
	this->discard = nullptr;
	this->discardshow = 0;
	for (int i=0; i < 4; i++)
		this->foundations[i] = nullptr;
}

static int print_cards(const Card *list)
{
	int n = 0;
	for (; list; list = list->next) {
		std::printf(" %c%s%s",
				list->visible ? 'v' : '?',
				list->suit.string().c_str(),
				list->numstring().c_str());
		n++;
	}
	std::printf(" (%d cards)\n", n);
	return n;
}

void Klon::debug_print() const
{
	int total = 0;

	std::printf("stock:");
	total += print_cards(this->stock);

	std::printf("discard:");
	total += print_cards(this->discard);

	for (int i=0; i < 4; i++) {
		std::printf("foundation %d:", i);
		total += print_cards(this->foundations[i]);
	}

	for (int i=0; i < 7; i++) {
		std::printf("tableau %d:", i);
		total += print_cards(this->tableau[i]);
	}

	std::printf("total: %d cards\n", total);
}

static void copy_cards(const Card *src, Card **dst, const Card *srccrd, Card **dstcrd, Card **list)
{
	*dst = nullptr;
	Card *top = nullptr;

	for (; src; src = src->next) {
		Card *dup = card_popbot(list);
		assert(dup);
		if (src == srccrd)
			*dstcrd = dup;

		*dup = *src;
		dup->next = nullptr;

		if (top)
			top->next = dup;
		else
			*dst = dup;
		top = dup;
	}
}

Card *Klon::dup(Klon& dst, const Card *srccrd) const
{
	Card *dstcrd = nullptr;

	Card *list = card_init_list(dst.allcards);
	copy_cards(this->stock, &dst.stock, srccrd, &dstcrd, &list);
	copy_cards(this->discard, &dst.discard, srccrd, &dstcrd, &list);
	dst.discardshow = this->discardshow;
	for (int i=0; i < 4; i++)
		copy_cards(this->foundations[i], &dst.foundations[i], srccrd, &dstcrd, &list);
	for (int i=0; i < 7; i++)
		copy_cards(this->tableau[i], &dst.tableau[i], srccrd, &dstcrd, &list);

	assert(list == nullptr);
	return dstcrd;
}

static bool card_in_some_tableau(const Klon& kln, const Card *crd)
{
	for (Card *bot : kln.tableau)
		for (Card *tabcrd = bot; tabcrd; tabcrd = tabcrd->next)
			if (tabcrd == crd)
				return true;
	return false;
}

bool Klon::canmove(const Card *crd, KlonCardPlace dst) const
{
	// taking cards stock to discard is handled by klon_stactodiscard() and not allowed here
	if (crd->next) {
		// the only way how a stack of multiple cards is allowed to move is tableau -> tableau
		// but for that, the bottommost moving card (crd) must be visible
		if (card_in_some_tableau(*this, crd) && KLON_IS_TABLEAU(dst) && crd->visible)
			goto tableau;
		return false;
	}

	if (crd->next || dst == KLON_STOCK || dst == KLON_DISCARD || !crd->visible)
		return false;

	if (KLON_IS_FOUNDATION(dst)) {
		Card *fnd = this->foundations[KLON_FOUNDATION_NUM(dst)];
		if (!fnd)
			return (crd->num == 1);

		fnd = card_top(fnd);
		return (crd->suit == fnd->suit && crd->num == fnd->num + 1);
	}

	tableau:
	if (KLON_IS_TABLEAU(dst)) {
		Card *tab = this->tableau[KLON_TABLEAU_NUM(dst)];
		if (!tab)
			return (crd->num == 13);

		tab = card_top(tab);
		return (crd->suit.color() != tab->suit.color() && crd->num == tab->num - 1);
	}

	assert(0);
}

// a double-linked list would make this easier but many other things harder
Card *Klon::detachcard(const Card *crd)
{
	std::vector<Card**> look4 = { &this->discard, &this->stock };
	for (int i=0; i < 4; i++)
		look4.push_back(&this->foundations[i]);
	for (int i=0; i < 7; i++)
		look4.push_back(&this->tableau[i]);

	for (Card **ptr : look4) {
		// special case: no card has crd as ->next
		if (*ptr == crd) {
			if (ptr == &this->discard)
				this->discardshow = 0;
			*ptr = nullptr;
			return nullptr;
		}

		for (Card *prv = *ptr; prv && prv->next; prv = prv->next)
			if (prv->next == crd) {
				if (ptr == &this->discard && this->discardshow > 1)
					this->discardshow--;
				prv->next = nullptr;
				return prv;
			}
	}

	throw std::logic_error("card not found");
}

void Klon::move(Card *crd, KlonCardPlace dst, bool raw)
{
	if (!raw)
		assert(this->canmove(crd, dst));

	Card *prv = this->detachcard(crd);

	// prv:
	//  * is nullptr, if crd was the bottommost card
	//  * has ->next==true, if moving only some of many visible cards on top of each other
	//  * has ->next==false, if crd was the bottommost VISIBLE card in the tableau list
	if (prv && !raw)
		prv->visible = true;

	Card **dstp;
	if (dst == KLON_STOCK)
		dstp = &this->stock;
	else if (dst == KLON_DISCARD)
		dstp = &this->discard;
	else if (KLON_IS_FOUNDATION(dst))
		dstp = &this->foundations[KLON_FOUNDATION_NUM(dst)];
	else if (KLON_IS_TABLEAU(dst))
		dstp = &this->tableau[KLON_TABLEAU_NUM(dst)];
	else
		throw std::logic_error("bad card place");

	card_pushtop(dstp, crd);
}

bool Klon::move2foundation(Card *card)
{
	if (!card)
		return false;

	for (int i=0; i < 4; i++)
		if (this->canmove(card, KLON_FOUNDATION(i))) {
			this->move(card, KLON_FOUNDATION(i), false);
			return true;
		}
	return false;
}

void Klon::stock2discard(int pick)
{
	assert(1 <= pick && pick <= 13*4 - (1+2+3+4+5+6+7));
	if (!this->stock) {
		for (Card *crd = this->discard; crd; crd = crd->next) {
			assert(crd->visible);
			crd->visible = false;
		}

		this->stock = this->discard;   // may be nullptr when all stock cards have been used
		this->discard = nullptr;
		this->discardshow = 0;
		return;
	}

	int i;
	for (i = 0; i < pick && this->stock; i++) {
		// the moved cards must be visible, but other stock cards aren't
		Card *pop = card_popbot(&this->stock);
		assert(!pop->visible);
		pop->visible = true;
		card_pushtop(&this->discard, pop);
	}

	// now there are i cards moved, and 0 <= i <= pick
	this->discardshow = i;
}

bool Klon::win() const
{
	for (Card *fnd : this->foundations) {
		int n = 0;
		for (Card *crd = fnd; crd; crd = crd->next)
			n++;

		// n can be >13 when cards are being moved
		if (n < 13)
			return false;
	}

	return true;
}
