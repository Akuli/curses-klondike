#include "card.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <vector>

std::string Card::number_string() const
{
	static constexpr std::array<std::string_view, 13> nstrs = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
	assert(1 <= this->number && this->number <= 13);
	return std::string(nstrs[this->number - 1]);
}

Card *cardlist::init(std::array<Card, 13*4>& cards)
{
	constexpr std::array<Suit, 4> suits = { Suit::CLUB, Suit::DIAMOND, Suit::HEART, Suit::SPADE };
	int i = 0;
	for (const Suit& s : suits) {
		for (int n = 1; n <= 13; n++) {
			cards[i++] = Card{ n, s };
		}
	}

	std::random_shuffle(std::begin(cards), std::end(cards));

	Card *last = std::end(cards) - 1;
	for (Card *ptr = std::begin(cards); ptr < last; ptr++)
		ptr->next = &ptr[1];
	last->next = nullptr;
	return &cards[0];
}

// crd can be nullptr
static Card *next_n_times(Card *crd, int n)
{
	while (n-- && crd)
		crd = crd->next;
	return crd;
}

Card *cardlist::top_n(Card *crd, int n)
{
	while (next_n_times(crd, n))
		crd = crd->next;
	return crd;
}

Card *cardlist::top(Card *crd)
{
	return cardlist::top_n(crd, 1);
}

Card *cardlist::pop_bottom(Card *& bot)
{
	Card *res = bot;
	bot = res->next;
	res->next = nullptr;
	return res;
}

void cardlist::push_top(Card *& list, Card *newtop)
{
	if (list)
		cardlist::top(list)->next = newtop;
	else
		list = newtop;
}
