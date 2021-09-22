#include "card.hpp"
#include "klon.hpp"
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

void Klon::init()
{
	Card *list = cardlist::init(this->allcards);
	for (int i=0; i < 7; i++) {
		Card *& tab = this->tableau[i];
		tab = nullptr;
		for (int j=0; j < i+1; j++)
			cardlist::push_top(tab, cardlist::pop_bottom(list));
		cardlist::top(tab)->visible = true;
	}

	this->stock = list;
	this->discard = nullptr;
	this->discardshow = 0;
	for (Card *& f : this->foundations)
		f = nullptr;
}

static void copy_cards(const Card *src, Card **dst, const Card *srccrd, Card **dstcrd, Card *& list)
{
	*dst = nullptr;
	Card *top = nullptr;

	for (; src; src = src->next) {
		Card *dup = cardlist::pop_bottom(list);
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

	Card *list = cardlist::init(dst.allcards);
	copy_cards(this->stock, &dst.stock, srccrd, &dstcrd, list);
	copy_cards(this->discard, &dst.discard, srccrd, &dstcrd, list);
	dst.discardshow = this->discardshow;
	for (int i=0; i < 4; i++)
		copy_cards(this->foundations[i], &dst.foundations[i], srccrd, &dstcrd, list);
	for (int i=0; i < 7; i++)
		copy_cards(this->tableau[i], &dst.tableau[i], srccrd, &dstcrd, list);

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

bool Klon::canmove(const Card *crd, CardPlace dst) const
{

	if (
		(
			// the only way how a stack of multiple cards is allowed to move is tableau -> tableau
			crd->next && !(
				dst.kind == CardPlace::TABLEAU &&
				card_in_some_tableau(*this, crd) &&
				crd->visible
			)
		)
		// taking cards stock to discard is handled by klon_stactodiscard() and not allowed here
		|| dst == CardPlace::stock()
		|| dst == CardPlace::discard()
		|| !crd->visible
	)
		return false;

	Card *fnd, *tab;
	switch(dst.kind) {
	case CardPlace::FOUNDATION:
		fnd = this->foundations[dst.num];
		if (!fnd)
			return (crd->num == 1);

		fnd = cardlist::top(fnd);
		return (crd->suit == fnd->suit && crd->num == fnd->num + 1);

	case CardPlace::TABLEAU:
		tab = this->tableau[dst.num];
		if (!tab)
			return (crd->num == 13);

		tab = cardlist::top(tab);
		return (crd->suit.color() != tab->suit.color() && crd->num == tab->num - 1);

	default:
		throw std::logic_error("this shouldn't happen");
	}
}

// a double-linked list would make this easier but many other things harder
Card *Klon::detachcard(const Card *crd)
{
	std::vector<Card**> look4 = { &this->discard, &this->stock };
	for (Card **p = std::begin(this->foundations); p < std::end(this->foundations); p++)
		look4.push_back(p);
	for (Card **p = std::begin(this->tableau); p < std::end(this->tableau); p++)
		look4.push_back(p);

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

void Klon::move(Card *crd, CardPlace dst, bool raw)
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
	switch(dst.kind) {
		case CardPlace::STOCK: dstp = &this->stock; break;
		case CardPlace::DISCARD: dstp = &this->discard; break;
		case CardPlace::FOUNDATION: dstp = &this->foundations[dst.num]; break;
		case CardPlace::TABLEAU: dstp = &this->tableau[dst.num]; break;
	}

	cardlist::push_top(*dstp, crd);
}

bool Klon::move2foundation(Card *card)
{
	if (!card)
		return false;

	for (int i=0; i < 4; i++)
		if (this->canmove(card, CardPlace::foundation(i))) {
			this->move(card, CardPlace::foundation(i), false);
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

		std::swap(this->stock, this->discard);
		this->discardshow = 0;
		return;
	}

	int i;
	for (i = 0; i < pick && this->stock; i++) {
		// the moved cards must be visible, but other stock cards aren't
		Card *pop = cardlist::pop_bottom(this->stock);
		assert(!pop->visible);
		pop->visible = true;
		cardlist::push_top(this->discard, pop);
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
