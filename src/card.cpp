#include "card.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iterator>
#include <vector>

Card *cardlist::init(Card (&arr)[13*4])
{
	int i = 0;
	for (Suit s : std::vector<Suit>{ Suit::CLUB, Suit::DIAMOND, Suit::HEART, Suit::SPADE }) {
		for (int n = 1; n <= 13; n++) {
			arr[i++] = Card{ n, s };
		}
	}

	std::random_shuffle(std::begin(arr), std::end(arr));

	Card *last = std::end(arr) - 1;
	for (Card *ptr = std::begin(arr); ptr < last; ptr++)
		ptr->next = &ptr[1];
	last->next = nullptr;
	return arr;
}

std::string Card::numstring() const
{
	static const std::string nstrs[13] = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
	assert(1 <= this->num && this->num <= 13);
	return nstrs[this->num - 1];
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
	if (list) {
		Card *top = cardlist::top(list);
		assert(!top->next);
		top->next = newtop;
	} else {
		list = newtop;
	}
}
