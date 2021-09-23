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
			cardlist::push_to_top(tab, cardlist::pop_from_bottom(list));
		cardlist::top(tab)->visible = true;
	}

	this->stock = list;
	this->discard = nullptr;
	this->discardshow = 0;
	for (Card *& f : this->foundations)
		f = nullptr;
}

static void copy_cards(const Card *src, Card **dest, const Card *source_card, Card **dstcrd, Card *& list)
{
	*dest = nullptr;
	Card *top = nullptr;

	for (; src; src = src->next) {
		Card *dup = cardlist::pop_from_bottom(list);
		if (src == source_card)
			*dstcrd = dup;

		*dup = *src;
		dup->next = nullptr;

		if (top)
			top->next = dup;
		else
			*dest = dup;
		top = dup;
	}
}

Card *Klon::dup(Klon& dest, const Card *source_card) const
{
	Card *dstcrd = nullptr;

	Card *list = cardlist::init(dest.allcards);
	copy_cards(this->stock, &dest.stock, source_card, &dstcrd, list);
	copy_cards(this->discard, &dest.discard, source_card, &dstcrd, list);
	dest.discardshow = this->discardshow;
	for (int i=0; i < 4; i++)
		copy_cards(this->foundations[i], &dest.foundations[i], source_card, &dstcrd, list);
	for (int i=0; i < 7; i++)
		copy_cards(this->tableau[i], &dest.tableau[i], source_card, &dstcrd, list);

	assert(list == nullptr);
	return dstcrd;
}

static bool card_in_some_tableau(const Klon& kln, const Card *card)
{
	for (Card *bot : kln.tableau)
		for (Card *tabcrd = bot; tabcrd; tabcrd = tabcrd->next)
			if (tabcrd == card)
				return true;
	return false;
}

bool Klon::can_move(const Card *card, CardPlace dest) const
{

	if (
		(
			// the only way how a stack of multiple cards is allowed to move is tableau -> tableau
			card->next && !(
				dest.kind == CardPlace::TABLEAU &&
				card_in_some_tableau(*this, card) &&
				card->visible
			)
		)
		// taking cards stock to discard is handled by klon_stactodiscard() and not allowed here
		|| dest == CardPlace::stock()
		|| dest == CardPlace::discard()
		|| !card->visible
	)
		return false;

	Card *fnd, *tab;
	switch(dest.kind) {
	case CardPlace::FOUNDATION:
		fnd = this->foundations[dest.number];
		if (!fnd)
			return (card->number == 1);

		fnd = cardlist::top(fnd);
		return (card->suit == fnd->suit && card->number == fnd->number + 1);

	case CardPlace::TABLEAU:
		tab = this->tableau[dest.number];
		if (!tab)
			return (card->number == 13);

		tab = cardlist::top(tab);
		return (card->suit.color() != tab->suit.color() && card->number == tab->number - 1);

	default:
		throw std::logic_error("this shouldn't happen");
	}
}

// a double-linked list would make this easier but many other things harder
Card *Klon::detach_card(const Card *card)
{
	std::vector<Card**> look4 = { &this->discard, &this->stock };
	for (Card **p = std::begin(this->foundations); p < std::end(this->foundations); p++)
		look4.push_back(p);
	for (Card **p = std::begin(this->tableau); p < std::end(this->tableau); p++)
		look4.push_back(p);

	for (Card **ptr : look4) {
		// special case: no card has card as ->next
		if (*ptr == card) {
			if (ptr == &this->discard)
				this->discardshow = 0;
			*ptr = nullptr;
			return nullptr;
		}

		for (Card *prv = *ptr; prv && prv->next; prv = prv->next)
			if (prv->next == card) {
				if (ptr == &this->discard && this->discardshow > 1)
					this->discardshow--;
				prv->next = nullptr;
				return prv;
			}
	}

	throw std::logic_error("card not found");
}

void Klon::move(Card *card, CardPlace dest, bool raw)
{
	if (!raw)
		assert(this->can_move(card, dest));

	Card *prv = this->detach_card(card);

	// prv:
	//  * is nullptr, if card was the bottommost card
	//  * has ->next==true, if moving only some of many visible cards on top of each other
	//  * has ->next==false, if card was the bottommost VISIBLE card in the tableau list
	if (prv && !raw)
		prv->visible = true;

	Card **dstp;
	switch(dest.kind) {
		case CardPlace::STOCK: dstp = &this->stock; break;
		case CardPlace::DISCARD: dstp = &this->discard; break;
		case CardPlace::FOUNDATION: dstp = &this->foundations[dest.number]; break;
		case CardPlace::TABLEAU: dstp = &this->tableau[dest.number]; break;
	}

	cardlist::push_to_top(*dstp, card);
}

bool Klon::move2foundation(Card *card)
{
	if (!card)
		return false;

	for (int i=0; i < 4; i++)
		if (this->can_move(card, CardPlace::foundation(i))) {
			this->move(card, CardPlace::foundation(i), false);
			return true;
		}
	return false;
}

void Klon::stock2discard(int pick)
{
	assert(1 <= pick && pick <= 13*4 - (1+2+3+4+5+6+7));
	if (!this->stock) {
		for (Card *card = this->discard; card; card = card->next) {
			assert(card->visible);
			card->visible = false;
		}

		std::swap(this->stock, this->discard);
		this->discardshow = 0;
		return;
	}

	int i;
	for (i = 0; i < pick && this->stock; i++) {
		// the moved cards must be visible, but other stock cards aren't
		Card *pop = cardlist::pop_from_bottom(this->stock);
		assert(!pop->visible);
		pop->visible = true;
		cardlist::push_to_top(this->discard, pop);
	}

	// now there are i cards moved, and 0 <= i <= pick
	this->discardshow = i;
}

bool Klon::win() const
{
	for (Card *fnd : this->foundations) {
		int n = 0;
		for (Card *card = fnd; card; card = card->next)
			n++;

		// n can be >13 when cards are being moved
		if (n < 13)
			return false;
	}

	return true;
}
