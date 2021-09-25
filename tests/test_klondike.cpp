#include "../src/card.hpp"
#include "../src/klondike.hpp"
#include <cassert>


static int count_cards(Card *fst, int *total, int *visible)
{
	int n = 0;
	for (Card *card = fst; card; card = card->next) {
		if (visible)
			*visible += !!card->visible;
		n++;
	}

	if (total)
		*total += n;
	return n;
}

void test_klondike_init()
{
	std::array<Card, 13*4> card_array;
	Klondike klon;
	klon.init(card_array);
	assert(klon.discardshow == 0);
	int total = 0;

	int svis = 0;
	assert(count_cards(klon.stock, &total, &svis) == 13*4 - (1+2+3+4+5+6+7));
	assert(svis == 0);

	assert(count_cards(klon.discard, &total, nullptr) == 0);

	for (Card *fnd : klon.foundations)
		assert(count_cards(fnd, &total, nullptr) == 0);
	for (int t=0; t < 7; t++) {
		int tvis = 0;
		assert(count_cards(klon.tableau[t], &total, &tvis) == t+1);
		assert(tvis == 1);
		assert(cardlist::top(klon.tableau[t])->visible);
	}

	assert(total == 13*4);
}

static bool cards_match(Card *list1, Card *list2)
{
	Card *a, *b;
	for (a = list1, b = list2; a || b; a = a->next, b = b->next) {
		assert(a != b);
		if (a == nullptr || b == nullptr) {
			// list lengths differ, both can't be null because a != b
			return false;
		}

		if (a->number != b->number || a->suit != b->suit || a->visible != b->visible)
			return false;
	}

	return true;
}

void test_klondike_dup()
{
	Klondike kln1, kln2;
	std::array<Card, 13*4> card_array1, card_array2;
	kln1.init(card_array1);
	Card *dupres = kln1.dup(kln2, kln1.tableau[2]->next, card_array2);
	assert(dupres == kln2.tableau[2]->next);

	assert(cards_match(kln1.stock, kln2.stock));
	assert(cards_match(kln1.discard, kln2.discard));
	for (int f=0; f < 4; f++)
		assert(cards_match(kln1.foundations[f], kln2.foundations[f]));
	for (int t=0; t < 7; t++)
		assert(cards_match(kln1.tableau[t], kln2.tableau[t]));
}

void test_klondike_canmove()
{
	Klondike klon;
	std::array<Card, 13*4> card_array;
	klon.init(card_array);

	// non-visible cards can never be moved
	assert(!klon.tableau[2]->visible);
	assert(!klon.can_move(klon.tableau[2], CardPlace::stock()));
	assert(!klon.can_move(klon.tableau[2], CardPlace::discard()));
	assert(!klon.can_move(klon.tableau[2], CardPlace::foundation(3)));
	assert(!klon.can_move(klon.tableau[2], CardPlace::tableau(3)));

	// discarding is implemented with klon_stocktodiscard(), not with klon_canmove()
	assert(!klon.can_move(klon.stock, CardPlace::discard()));

	// TODO: test rest of the code? problem is, how do i find e.g. ♠A or ♥Q
}

// creates a game where a move is possible, sets move data to mvcrd and mvdst
static void init_movable_kln(Klondike *klon, int *srctab, int *dsttab, std::array<Card, 13*4>& card_array)
{
	while (true) {
		klon->init(card_array);

		for (int i=0; i < 7; i++) {
			for (int j=0; j < 7; j++) {
				if (i == j)
					continue;
				if (klon->can_move(cardlist::top(klon->tableau[i]), CardPlace::tableau(j))) {
					*srctab = i;
					*dsttab = j;
					return;
				}
			}
		}
	}
}

void test_klondike_move()
{
	Klondike klon;
	std::array<Card, 13*4> card_array;
	int srctab, dsttab;
	init_movable_kln(&klon, &srctab, &dsttab, card_array);

	klon.move(cardlist::top(klon.tableau[srctab]), CardPlace::tableau(dsttab), false);
	for (int i=0; i < 7; i++) {
		int ncrd = i+1 - (i == srctab) + (i == dsttab);
		int visible = 0;
		assert(count_cards(klon.tableau[i], nullptr, &visible) == ncrd);

		if (i == dsttab)
			assert(visible == 2);
		else if (i == 0 && srctab == 0)
			assert(visible == 0);
		else
			assert(visible == 1);

		if (klon.tableau[i])
			assert(cardlist::top(klon.tableau[i])->visible);
	}
}

static void discard_check(Klondike klon, int ndiscarded, int ds)
{
	assert(klon.discardshow == ds);

	int svis = 0;
	assert(count_cards(klon.stock, nullptr, &svis) == 13*4 - (1+2+3+4+5+6+7) - ndiscarded);
	assert(svis == 0);

	int dvis = 0;
	assert(count_cards(klon.discard, nullptr, &dvis) == ndiscarded);
	assert(dvis == ndiscarded);
}

void test_klondike_stock2discard()
{
	Klondike klon;
	std::array<Card, 13*4> card_array;
	klon.init(card_array);
	discard_check(klon, 0, 0);

	Card *savestock = klon.stock;

	klon.stock2discard(1);
	discard_check(klon, 1, 1);

	klon.stock2discard(2);
	discard_check(klon, 3, 2);

	for (int n=4; n <= 13*4 - (1+2+3+4+5+6+7) - 1; n++) {
		klon.stock2discard(1);
		discard_check(klon, n, 1);
	}

	klon.stock2discard(1);
	discard_check(klon, 13*4 - (1+2+3+4+5+6+7), 1);

	klon.stock2discard(13*4 - (1+2+3+4+5+6+7));  // 11 is an arbitrary number i picked
	discard_check(klon, 0, 0);

	// make sure that the order of the cards doesn't reverse
	assert(klon.stock == savestock);

	// TODO: test with less than 13*4 - (1+2+3+4+5+6+7) stock cards? e.g. 0 stock cards
}
