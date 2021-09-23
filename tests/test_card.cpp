#include "../src/card.hpp"
#include <cassert>
#include <string>
#include <vector>

void test_card_numstring()
{
	struct CardStrTest { int number; std::string number_string; };
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

	for (const CardStrTest& test : tests) {
		for (Suit s : std::vector<Suit>{ Suit::CLUB, Suit::DIAMOND, Suit::HEART, Suit::SPADE }) {
			for (bool visible : std::vector<bool>{ true, false }) {
				assert(( Card{ test.number, s, visible }.number_string() == test.number_string ));
			}
		}
	}
}

void test_cardlist_init()
{
	int got[4][13] = {0};

	std::array<Card, 13*4> cards;
	Card *fst = cardlist::init(cards);
	for (Card *card = fst; card; card = card->next)
		got[card->suit][card->number-1]++;

	for (auto& row : got)
		for (int val : row)
			assert(val == 1);
}

static void abccards(Card *a, Card *b, Card *c, bool link)
{
	a->number = 1;
	a->suit = Suit::SPADE;
	a->visible = true;
	a->next = link ? b : nullptr;

	b->number = 2;
	b->suit = Suit::DIAMOND;
	b->visible = false;
	b->next = link ? c : nullptr;

	c->number = 3;
	c->suit = Suit::HEART;
	c->visible = true;
	c->next = nullptr;
}

void test_cardlist_top()
{
	Card a, b, c;
	abccards(&a, &b, &c, true);

	assert(cardlist::top(&a) == &c);
	assert(cardlist::top(cardlist::top(&a)) == &c);

	assert(cardlist::top_n(&a, 0) == nullptr);
	assert(cardlist::top_n(&a, 1) == &c);
	assert(cardlist::top_n(&a, 2) == &b);
	assert(cardlist::top_n(&a, 3) == &a);
	assert(cardlist::top_n(&a, 4) == &a);

	assert(cardlist::top_n(&b, 0) == nullptr);
	assert(cardlist::top_n(&b, 1) == &c);
	assert(cardlist::top_n(&b, 2) == &b);
	assert(cardlist::top_n(&b, 3) == &b);
	assert(cardlist::top_n(&b, 4) == &b);

	assert(cardlist::top_n(&c, 0) == nullptr);
	assert(cardlist::top_n(&c, 1) == &c);
	assert(cardlist::top_n(&c, 2) == &c);
	assert(cardlist::top_n(&c, 3) == &c);
	assert(cardlist::top_n(&c, 4) == &c);
}

void test_cardlist_pop()
{
	Card a, b, c;
	abccards(&a, &b, &c, true);

	Card *p = &a;
	assert(cardlist::pop_from_bottom(p) == &a);
	assert(p == &b);
}

void test_cardlist_push()
{
	Card a, b, c;
	abccards(&a, &b, &c, false);
	Card *p = nullptr;

	cardlist::push_to_top(p, &a);
	assert(p == &a);
	assert(!p->next);

	cardlist::push_to_top(p, &b);
	assert(p == &a);
	assert(p->next == &b);
	assert(!p->next->next);
}
