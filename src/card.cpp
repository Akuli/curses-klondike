#include "card.hpp"
#include <algorithm>
#include <vector>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.hpp"

struct Card *card_createallshuf(void)
{
	std::vector<struct Card *> cards;
	for (Suit s : std::vector<Suit>{ Suit::CLUB, Suit::DIAMOND, Suit::HEART, Suit::SPADE }) {
		for (int n = 1; n <= 13; n++) {
			struct Card *crd = (struct Card *)malloc(sizeof(struct Card));
			if (!crd)
				fatal_error("malloc() failed");

			*crd = Card{ n, s };
			cards.push_back(crd);
		}
	}

	std::random_shuffle(cards.begin(), cards.end());

	for (unsigned j = 0; j < cards.size(); j++)
		cards[j]->next = (j+1 == cards.size() ? nullptr : cards[j+1]);
	return cards[0];
}

void card_free(struct Card *crd)
{
	struct Card *nxt;
	for (; crd; crd = nxt) {
		nxt = crd->next;
		free(crd);
	}
}

std::string card_numstr(struct Card crd)
{
	static const char *nstrs[] = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
	assert(sizeof(nstrs)/sizeof(nstrs[0]) == 13);
	assert(1 <= crd.num && crd.num <= 13);
	return nstrs[crd.num - 1];
}

void card_debug(struct Card crd)
{
	printf("%s%s\n", crd.suit.string().c_str(), card_numstr(crd).c_str());
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
