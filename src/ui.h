#ifndef UI_H
#define UI_H

#include <curses.h>
#include "card.h"   // IWYU pragma: keep
#include "sol.h"

/*
represents the card pile or location that the user has selected
a pointer to struct Card is not enough because it's possible to select a
place where cards can be put but currently contains no cards

possible values:

	.card = NULL, .place = 0
		nothing selected

	.card = NULL, .place = SOL_STOCK
		stock selected

	.card = card_top(sol->discard), .place = SOL_DISCARD
		discard selected

	.card = card_top(sol->foundations[n]), .place = SOL_FOUNDATION(n)
		nth foundation selected

	.card = sol->tableau[n] or some of its ->nexts, .card is visible, .place = SOL_TABLEAU(n)
		nth tableau selected, including the specified card and all its ->nexts

	.card = NULL, .place = SOL_TABLEAU(n)
		tableau n selected, but there are no cards in that tableau
*/
struct UiSelection {
	struct Card *card;
	SolCardPlace place;
};

// draws sol on win
void ui_drawsol(WINDOW *win, struct Sol sol, struct UiSelection sel);

#endif  // UI_H
