#ifndef UI_H
#define UI_H

#include <curses.h>
#include "card.h"   // IWYU pragma: keep
#include "sol.h"

/*
represents the card pile or location that the user has selected
a pointer to struct Card is not enough because it's possible to select a
place where cards can be put, but that currently contains no cards

possible values:

	.card = NULL, .place = 0
		nothing selected

	.card = NULL, .place = SOL_STOCK
		stock selected

	.card = NULL, .place = SOL_DISCARD
		discard selected

	.card = NULL, .place = SOL_FOUNDATION(n)
		n'th foundation selected

	.card != NULL, .place = SOL_TABLEAU(n)      where 0 <= n < 4
		tableau n selected, bottommost selected card is the card

	.card = NULL, .place = SOL_TABLEAU(n)       where 0 <= n < 4
		tableau n selected, there are no cards in that tableau
*/
struct UiSelection {
	struct Card *card;   // NULL for nothing selected
	SolCardPlace place;  // 0 for nothing selected
};

// draws sol on win
void ui_drawsol(WINDOW *win, struct Sol sol, struct UiSelection sel);

#endif  // UI_H
