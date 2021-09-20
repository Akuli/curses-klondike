#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../src/card.hpp"
#include "../src/klon.hpp"
#include "util.hpp"


static int count_cards(Card *fst, int *total, int *visible)
{
	int n = 0;
	for (Card *crd = fst; crd; crd = crd->next) {
		if (visible)
			*visible += !!crd->visible;
		n++;
	}

	if (total)
		*total += n;
	return n;
}

TEST(klon_init_free)
{
	Klon kln;
	kln.init();
	assert(kln.discardshow == 0);
	int total = 0;

	int svis = 0;
	assert(count_cards(kln.stock, &total, &svis) == 13*4 - (1+2+3+4+5+6+7));
	assert(svis == 0);

	assert(count_cards(kln.discard, &total, NULL) == 0);

	for (int f=0; f < 4; f++)
		assert(count_cards(kln.foundations[f], &total, NULL) == 0);
	for (int t=0; t < 7; t++) {
		int tvis = 0;
		assert(count_cards(kln.tableau[t], &total, &tvis) == t+1);
		assert(tvis == 1);
		assert(card_top(kln.tableau[t])->visible);
	}

	assert(total == 13*4);
}

static bool cards_match(Card *list1, Card *list2)
{
	Card *a, *b;
	for (a = list1, b = list2; a || b; a = a->next, b = b->next) {
		assert(a != b);
		if (a == NULL || b == NULL) {
			// list lengths differ, both can't be null because a != b
			return false;
		}

		if (a->num != b->num || a->suit != b->suit || a->visible != b->visible)
			return false;
	}

	return true;
}

TEST(klon_dup)
{
	Klon kln1, kln2;
	kln1.init();
	Card *dupres = kln1.dup(kln2, kln1.tableau[2]->next);
	assert(dupres == kln2.tableau[2]->next);

	assert(cards_match(kln1.stock, kln2.stock));
	assert(cards_match(kln1.discard, kln2.discard));
	for (int f=0; f < 4; f++)
		assert(cards_match(kln1.foundations[f], kln2.foundations[f]));
	for (int t=0; t < 7; t++)
		assert(cards_match(kln1.tableau[t], kln2.tableau[t]));
}

TEST(klon_canmove)
{
	Klon kln;
	kln.init();

	// non-visible cards can never be moved
	assert(!kln.tableau[2]->visible);
	assert(!kln.canmove(kln.tableau[2], CardPlace(CardPlaceKind::STOCK)));
	assert(!kln.canmove(kln.tableau[2], CardPlace(CardPlaceKind::DISCARD)));
	assert(!kln.canmove(kln.tableau[2], CardPlace(CardPlaceKind::FOUNDATION, 3)));
	assert(!kln.canmove(kln.tableau[2], CardPlace(CardPlaceKind::TABLEAU, 3)));

	// discarding is implemented with klon_stocktodiscard(), not with klon_canmove()
	assert(!kln.canmove(kln.stock, CardPlace(CardPlaceKind::DISCARD)));

	// TODO: test rest of the code? problem is, how do i find e.g. ♠A or ♥Q
}

// creates a game where a move is possible, sets move data to mvcrd and mvdst
static void init_movable_kln(Klon *kln, int *srctab, int *dsttab)
{
	while (true) {
		kln->init();

		for (int i=0; i < 7; i++) {
			for (int j=0; j < 7; j++) {
				if (i == j)
					continue;

				Card *itop = card_top(kln->tableau[i]);
				if (kln->canmove(itop, CardPlace(CardPlaceKind::TABLEAU, j))) {
					*srctab = i;
					*dsttab = j;
					return;
				}
			}
		}
	}
}

TEST(klon_move)
{
	Klon kln;
	int srctab, dsttab;
	init_movable_kln(&kln, &srctab, &dsttab);

	kln.move(card_top(kln.tableau[srctab]), CardPlace(CardPlaceKind::TABLEAU, dsttab), false);
	for (int i=0; i < 7; i++) {
		int ncrd = i+1 - (i == srctab) + (i == dsttab);
		int visible = 0;
		assert(count_cards(kln.tableau[i], NULL, &visible) == ncrd);

		if (i == dsttab)
			assert(visible == 2);
		else if (i == 0 && srctab == 0)
			assert(visible == 0);
		else
			assert(visible == 1);

		if (kln.tableau[i])
			assert(card_top(kln.tableau[i])->visible);
	}
}

static void discard_check(Klon kln, int ndiscarded, int ds)
{
	assert(kln.discardshow == ds);

	int svis = 0;
	assert(count_cards(kln.stock, NULL, &svis) == 13*4 - (1+2+3+4+5+6+7) - ndiscarded);
	assert(svis == 0);

	int dvis = 0;
	assert(count_cards(kln.discard, NULL, &dvis) == ndiscarded);
	assert(dvis == ndiscarded);
}

TEST(klon_stock2discard)
{
	Klon kln;
	kln.init();
	discard_check(kln, 0, 0);

	Card *savestock = kln.stock;

	kln.stock2discard(1);
	discard_check(kln, 1, 1);

	kln.stock2discard(2);
	discard_check(kln, 3, 2);

	for (int n=4; n <= 13*4 - (1+2+3+4+5+6+7) - 1; n++) {
		kln.stock2discard(1);
		discard_check(kln, n, 1);
	}

	kln.stock2discard(1);
	discard_check(kln, 13*4 - (1+2+3+4+5+6+7), 1);

	kln.stock2discard(13*4 - (1+2+3+4+5+6+7));  // 11 is an arbitrary number i picked
	discard_check(kln, 0, 0);

	// make sure that the order of the cards doesn't reverse
	assert(kln.stock == savestock);

	// TODO: test with less than 13*4 - (1+2+3+4+5+6+7) stock cards? e.g. 0 stock cards
}
