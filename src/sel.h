#ifndef SEL_H
#define SEL_H

#include <stdbool.h>
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
struct Sel {
	struct Card *card;
	KlonCardPlace place;
};

enum SelDirection { SEL_LEFT, SEL_RIGHT, SEL_UP, SEL_DOWN };

// selects the topmost card at plc
void sel_byplace(struct Klon kln, struct Sel *sel, KlonCardPlace plc);

// select another card at left, right, top or bottom, if possible
// NOT for moving the card
void sel_anothercard(struct Klon kln, struct Sel *sel, enum SelDirection dir);

// sel_anothercard(), but called when the user is moving a card around
// sel is what is being moved (card not NULL), mv is where the card is being moved
void sel_anothercardmv(struct Klon kln, struct Sel sel, enum SelDirection dir, KlonCardPlace *mv);

// if sel is in tableau and possible to select more/less cards in that tableau item, do that
// returns true if something was done, false otherwise
bool sel_more(struct Klon kln, struct Sel *sel);
bool sel_less(struct Klon kln, struct Sel *sel);

// called when the user is done with dragging a card, moves the card if possible and resets sel
void sel_endmv(struct Klon *kln, struct Sel *sel, KlonCardPlace mv);

#endif  // SEL_H
