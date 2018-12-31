#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <src/card.h>
#include <src/sol.h>
#include "util.h"


TEST(sol_cardplace)
{
	assert(sizeof(SolCardPlace) == 1);
	bool seen[256] = {false};

	// make sure that there are no duplicate values
#define dupcheck(val) do{ unsigned char u = (val); assert(!seen[u]); seen[u] = true; }while(0)
	dupcheck(SOL_STOCK);
	dupcheck(SOL_DISCARD);

	for (int f=0; f < 4; f++) {
		dupcheck(SOL_FOUNDATION(f));
		assert(SOL_FOUNDATION_NUM(SOL_FOUNDATION(f)) == f);
	}
	for (int t=0; t < 7; t++) {
		dupcheck(SOL_TABLEAU(t));
#undef dupcheck
		assert(SOL_TABLEAU_NUM(SOL_TABLEAU(t)) == t);
	}
}

static int count_cards(struct Card *fst, int *total, int *visible)
{
	int n = 0;
	for (struct Card *crd = fst; crd; crd = crd->next) {
		if (visible)
			*visible += !!crd->visible;
		n++;
	}

	if (total)
		*total += n;
	return n;
}

TEST(sol_init_free)
{
	struct Sol sol;
	sol_init(&sol, card_createallshuf());
	int total = 0;

	int svis = 0;
	assert(count_cards(sol.stock, &total, &svis) == 13*4 - (1+2+3+4+5+6+7));
	assert(svis == 0);

	assert(count_cards(sol.discard, &total, NULL) == 0);

	for (int f=0; f < 4; f++)
		assert(count_cards(sol.foundations[f], &total, NULL) == 0);
	for (int t=0; t < 7; t++) {
		int tvis = 0;
		assert(count_cards(sol.tableau[t], &total, &tvis) == t+1);
		assert(tvis == 1);
		assert(card_top(sol.tableau[t])->visible);
	}

	assert(total == 13*4);
	sol_free(sol);
}

static bool cards_match(struct Card *list1, struct Card *list2)
{
	struct Card *a, *b;
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

TEST(sol_dup)
{
	struct Sol sol1, sol2;
	sol_init(&sol1, card_createallshuf());
	sol_dup(sol1, &sol2);

	assert(cards_match(sol1.stock, sol2.stock));
	assert(cards_match(sol1.discard, sol2.discard));
	for (int f=0; f < 4; f++)
		assert(cards_match(sol1.foundations[f], sol2.foundations[f]));
	for (int t=0; t < 7; t++)
		assert(cards_match(sol1.tableau[t], sol2.tableau[t]));

	sol_free(sol1);
	sol_free(sol2);
}

TEST(sol_canmove)
{
	struct Sol sol;
	sol_init(&sol, card_createallshuf());

	// non-visible cards can never be moved
	assert(!sol.tableau[2][0].visible);
	assert(!sol_canmove(sol, &sol.tableau[2][0], SOL_STOCK));
	assert(!sol_canmove(sol, &sol.tableau[2][0], SOL_DISCARD));
	assert(!sol_canmove(sol, &sol.tableau[2][0], SOL_FOUNDATION(3)));
	assert(!sol_canmove(sol, &sol.tableau[2][0], SOL_TABLEAU(3)));

	// discarding is not implemented with sol_canmove
	assert(!sol_canmove(sol, sol.stock, SOL_DISCARD));

	// TODO: test rest of the code? problem is, how do i find e.g. ♠A or ♥Q

	sol_free(sol);
}

// creates a game where a move is possible, sets move data to mvcrd and mvdst
static void init_movable_sol(struct Sol *sol, int *srctab, int *dsttab)
{
	while (true) {
		sol_init(sol, card_createallshuf());

		for (int i=0; i < 7; i++)
		for (int j=0; j < 7; j++)
		{
			if (i == j)
				continue;

			struct Card *itop = card_top(sol->tableau[i]);
			if (sol_canmove( *sol, itop, SOL_TABLEAU(j) )) {
				*srctab = i;
				*dsttab = j;
				return;
			}
		}

		sol_free(*sol);
	}
}

TEST(sol_move)
{
	struct Sol sol;
	int srctab, dsttab;
	init_movable_sol(&sol, &srctab, &dsttab);

	sol_move(&sol, card_top(sol.tableau[srctab]), SOL_TABLEAU(dsttab));
	for (int i=0; i < 7; i++) {
		int ncrd = i+1 - (i == srctab) + (i == dsttab);
		int visible = 0;
		assert(count_cards(sol.tableau[i], NULL, &visible) == ncrd);

		assert(visible == (i==dsttab ? 2 : 1));
		assert(card_top(sol.tableau[i])->visible);
	}

	sol_free(sol);
}

static void discard_check(struct Sol sol, int ndiscarded)
{
	int svis = 0;
	assert(count_cards(sol.stock, NULL, &svis) == 13*4 - (1+2+3+4+5+6+7) - ndiscarded);
	assert(svis == 0);

	int dvis = 0;
	assert(count_cards(sol.discard, NULL, &dvis) == ndiscarded);
	assert(dvis == ndiscarded);
}

TEST(sol_stocktodiscard)
{
	struct Sol sol;
	sol_init(&sol, card_createallshuf());
	discard_check(sol, 0);

	struct Card *savestock = sol.stock;

	sol_stocktodiscard(&sol);
	discard_check(sol, 1);

	sol_stocktodiscard(&sol);
	discard_check(sol, 2);

	for (int n=3; n <= 13*4 - (1+2+3+4+5+6+7) - 1; n++) {
		sol_stocktodiscard(&sol);
		discard_check(sol, n);
	}

	sol_stocktodiscard(&sol);
	discard_check(sol, 13*4 - (1+2+3+4+5+6+7));

	sol_stocktodiscard(&sol);
	discard_check(sol, 0);

	// make sure that the order of the cards doesn't reverse
	assert(sol.stock == savestock);

	// TODO: test with less than 13*4 - (1+2+3+4+5+6+7) stock cards? e.g. 0 stock cards

	sol_free(sol);
}
