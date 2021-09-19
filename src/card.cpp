#include "card.hpp"
#include <algorithm>
#include <memory>
#include <vector>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.hpp"

Card *card_init_list(Card (&arr)[13*4])
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

void card_debug(struct Card crd)
{
	printf("%s%s\n", crd.suit.string().c_str(), crd.numstring().c_str());
}

// crd can be NULL
static struct Card *next_n_times(struct Card *crd, unsigned int n)
{
	for (unsigned int i=0; i<n && crd; i++)
		crd = crd->next;
	return crd;
}

struct Card *card_tops(struct Card *crd, unsigned int n)
{
	while (next_n_times(crd, n))
		crd = crd->next;
	return crd;
}

struct Card *card_top(struct Card *crd)
{
	return card_tops(crd, 1);
}

struct Card *card_popbot(struct Card **bot)
{
	struct Card *res = *bot;  // c funniness: res and bot are different types, but this is correct
	*bot = res->next;
	res->next = NULL;
	return res;
}

void card_pushtop(struct Card **list, struct Card *newtop)
{
	if (*list) {
		struct Card *top = card_top(*list);
		assert(!top->next);
		top->next = newtop;
	} else
		*list = newtop;
}