#include "card.hpp"
#include "klon.hpp"
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

void Klondike::init(std::array<Card, 13*4>& card_array)
{
	Card *list = cardlist::init(card_array);
	for (int i=0; i < 7; i++) {
		Card *& table = this->tableau[i];
		table = nullptr;
		for (int j=0; j < i+1; j++)
			cardlist::push_to_top(table, cardlist::pop_from_bottom(list));
		cardlist::top(table)->visible = true;
	}

	this->stock = list;
	this->discard = nullptr;
	this->discardshow = 0;
	for (Card *& foundation : this->foundations)
		foundation = nullptr;
}

static void copy_cards(const Card *src, Card *& dest, const Card *source_card, Card **dest_card, Card *& list)
{
	dest = nullptr;
	for (; src; src = src->next) {
		Card *copy = cardlist::pop_from_bottom(list);
		if (src == source_card)
			*dest_card = copy;

		*copy = *src;
		copy->next = nullptr;
		cardlist::push_to_top(dest, copy);
	}
}

Card *Klondike::dup(Klondike& dest, const Card *source_card, std::array<Card, 13*4>& card_array) const
{
	Card *dest_card = nullptr;

	Card *list = cardlist::init(card_array);
	copy_cards(this->stock, dest.stock, source_card, &dest_card, list);
	copy_cards(this->discard, dest.discard, source_card, &dest_card, list);
	dest.discardshow = this->discardshow;
	for (int i=0; i < 4; i++)
		copy_cards(this->foundations[i], dest.foundations[i], source_card, &dest_card, list);
	for (int i=0; i < 7; i++)
		copy_cards(this->tableau[i], dest.tableau[i], source_card, &dest_card, list);

	assert(list == nullptr);  // all cards used
	return dest_card;
}

static bool card_in_some_tableau(const Klondike& klon, const Card *card)
{
	for (Card *bot : klon.tableau)
		for (Card *tabcrd = bot; tabcrd; tabcrd = tabcrd->next)
			if (tabcrd == card)
				return true;
	return false;
}

bool Klondike::can_move(const Card *card, CardPlace dest) const
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

	Card *foundation, *table;
	switch(dest.kind) {
	case CardPlace::FOUNDATION:
		foundation = this->foundations[dest.number];
		if (!foundation)
			return (card->number == 1);

		foundation = cardlist::top(foundation);
		return (card->suit == foundation->suit && card->number == foundation->number + 1);

	case CardPlace::TABLEAU:
		table = this->tableau[dest.number];
		if (!table)
			return (card->number == 13);

		table = cardlist::top(table);
		return (card->suit.color() != table->suit.color() && card->number == table->number - 1);

	default:
		throw std::logic_error("this shouldn't happen");
	}
}

// a double-linked list would make this easier but many other things harder
Card *Klondike::detach_card(const Card *card)
{
	std::vector<Card**> card_lists = { &this->discard, &this->stock };
	for (Card **p = this->foundations.begin(); p < this->foundations.end(); p++)
		card_lists.push_back(p);
	for (Card **p = this->tableau.begin(); p < this->tableau.end(); p++)
		card_lists.push_back(p);

	for (Card **list : card_lists) {
		// special case: no card has card as ->next
		if (*list == card) {
			if (list == &this->discard)
				this->discardshow = 0;
			*list = nullptr;
			return nullptr;
		}

		for (Card *prev = *list; prev && prev->next; prev = prev->next)
			if (prev->next == card) {
				if (list == &this->discard && this->discardshow > 1)
					this->discardshow--;
				prev->next = nullptr;
				return prev;
			}
	}

	throw std::logic_error("card not found");
}

void Klondike::move(Card *card, CardPlace dest, bool raw)
{
	if (!raw)
		assert(this->can_move(card, dest));

	Card *prev = this->detach_card(card);

	// prev:
	//  * is nullptr, if card was the bottommost card
	//  * has ->next==true, if moving only some of many visible cards on top of each other
	//  * has ->next==false, if card was the bottommost VISIBLE card in the tableau list
	if (prev && !raw)
		prev->visible = true;

	Card **dest_pointer;
	switch(dest.kind) {
		case CardPlace::STOCK: dest_pointer = &this->stock; break;
		case CardPlace::DISCARD: dest_pointer = &this->discard; break;
		case CardPlace::FOUNDATION: dest_pointer = &this->foundations[dest.number]; break;
		case CardPlace::TABLEAU: dest_pointer = &this->tableau[dest.number]; break;
	}

	cardlist::push_to_top(*dest_pointer, card);
}

bool Klondike::move2foundation(Card *card)
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

void Klondike::stock2discard(int pick)
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

bool Klondike::win() const
{
	for (Card *foundation : this->foundations) {
		int n = 0;
		for (Card *card = foundation; card; card = card->next)
			n++;

		// n can be >13 when cards are being moved
		if (n < 13)
			return false;
	}

	return true;
}
