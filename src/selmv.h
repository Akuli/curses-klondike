#ifndef SELMV_H
#define SELMV_H

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

// represents card being moved src --> dst
struct Mv {
	struct Card *card;
	KlonCardPlace src;
	KlonCardPlace dst;
};

struct SelMv {
	struct Sel sel;
	struct Mv mv;
	bool ismv;
};

enum SelDirection { SEL_LEFT, SEL_RIGHT, SEL_UP, SEL_DOWN };

// selects the topmost card at plc
void selmv_byplace(struct Klon kln, struct SelMv *selmv, KlonCardPlace plc);

// select another card at left, right, top or bottom, if possible
void selmv_anothercard(struct Klon kln, struct SelMv *selmv, enum SelDirection dir);

// if sel is in tableau and possible to select more/less cards in that tableau item, do that
// returns true if something was done, false otherwise
bool sel_more(struct Klon kln, struct Sel *sel);
bool sel_less(struct Klon kln, struct Sel *sel);

// sets selmv->ismv to true and updates other things correctly
void selmv_beginmv(struct SelMv *selmv);

// called when the user is done with dragging a card, moves the card if possible and resets sel
void selmv_endmv(struct Klon *kln, struct SelMv *selmv);

#endif  // SELMV_H
