#ifndef UI_H
#define UI_H

#include <curses.h>
#include "card.h"   // IWYU pragma: keep
#include "klon.h"

/*
represents the card pile or location that the user has selected
a pointer to struct Card is not enough because it's possible to select a
place where cards can be put but currently contains no cards

possible values:

	.card = NULL, .place = 0
		nothing selected

	.card = NULL, .place = KLON_STOCK
		stock selected

	.card = card_top(kln->discard), .place = KLON_DISCARD
		discard selected

	.card = card_top(kln->foundations[n]), .place = KLON_FOUNDATION(n)
		nth foundation selected

	.card = kln->tableau[n] or some of its ->nexts, .card is visible, .place = KLON_TABLEAU(n)
		nth tableau selected, including the specified card and all its ->nexts

	.card = NULL, .place = KLON_TABLEAU(n)
		tableau n selected, but there are no cards in that tableau
*/
struct UiSelection {
	struct Card *card;
	KlonCardPlace place;
};

// draws kln on win
void ui_drawklon(WINDOW *win, struct Klon kln, struct UiSelection sel);

#endif  // UI_H
