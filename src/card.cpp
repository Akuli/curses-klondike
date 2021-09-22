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

	arr[13*4 - 1].next = nullptr;
	for (int i = 13*4 - 2; i >= 0; i--)
		arr[i].next = &arr[i+1];
	return &arr[0];
}

std::string Card::numstring() const
{
	static const std::string nstrs[13] = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
	assert(1 <= this->num && this->num <= 13);
	return nstrs[this->num - 1];
}

void Card::debug_print() const
{
	std::printf("%s%s\n", this->suit.string().c_str(), this->numstring().c_str());
}

// crd can be nullptr
static Card *next_n_times(Card *crd, int n)
{
	for (int i=0; i<n && crd; i++)
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

Card *cardlist::pop_bottom(Card **bot)
{
	Card *res = *bot;  // c funniness: res and bot are different types, but this is correct
	*bot = res->next;
	res->next = nullptr;
	return res;
}

void cardlist::push_top(Card **list, Card *newtop)
{
	if (*list) {
		Card *top = cardlist::top(*list);
		assert(!top->next);
		top->next = newtop;
	} else
		*list = newtop;
}
