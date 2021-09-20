#include <vector>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "util.hpp"
#include "../src/card.hpp"

TEST(card_createallshuf_gives_all_52_cards)
{
	int got[4][13] = {0};

	Card cards[13*4];
	Card *fst = card_init_list(cards);
	for (Card *crd = fst; crd; crd = crd->next)
		got[(int)crd->suit][crd->num-1]++;

	for (int i=0; i < 4; i++)
		for (int j=0; j < 7; j++)
			assert(got[i][j] == 1);
}

struct CardStrTest { int num; std::string numstr; };

TEST(card_str)
{
	CardStrTest tests[] = {
		{1, "A"},
		{2, "2"},
		{3, "3"},
		{9, "9"},
		{10, "10"},
		{11, "J"},
		{12, "Q"},
		{13, "K"}
	};

	// nested for loops without nested indentations or braces
	for (const CardStrTest& test : tests)
	for (Suit s : std::vector<Suit>{ Suit::CLUB, Suit::DIAMOND, Suit::HEART, Suit::SPADE })
	for (int visible=0; visible < 2; visible++)
	{
		Card crd = { test.num, s, (bool)visible };
		assert(crd.numstring() == test.numstr);
	}
}

static void abccards(Card *a, Card *b, Card *c, bool link)
{
	a->num = 1;
	a->suit = Suit::SPADE;
	a->visible = true;
	a->next = link ? b : nullptr;

	b->num = 2;
	b->suit = Suit::DIAMOND;
	b->visible = false;
	b->next = link ? c : nullptr;

	c->num = 3;
	c->suit = Suit::HEART;
	c->visible = true;
	c->next = nullptr;
}

TEST(card_top)
{
	Card a, b, c;
	abccards(&a, &b, &c, true);

	assert(card_top(&a) == &c);
	assert(card_top(card_top(&a)) == &c);
}

TEST(card_popbot)
{
	Card a, b, c;
	abccards(&a, &b, &c, true);

	Card *p = &a;
	assert(card_popbot(&p) == &a);
	assert(p == &b);
}

TEST(card_pushtop)
{
	Card a, b, c;
	abccards(&a, &b, &c, false);
	Card *p = nullptr;

	card_pushtop(&p, &a);
	assert(p == &a);
	assert(!p->next);

	card_pushtop(&p, &b);
	assert(p == &a);
	assert(p->next == &b);
	assert(!p->next->next);
}
