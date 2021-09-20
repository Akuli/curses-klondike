#ifndef SELMV_H
#define SELMV_H

#include <stdbool.h>
#include "klon.hpp"

/*
represents the card pile or location that the user has selected
a pointer to Card is not enough because it's possible to select a
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
	Card *card;
	CardPlace place;
};

// represents card being moved src --> dst
struct Mv {
	Card *card;
	CardPlace src;
	CardPlace dst;
};

struct SelMv {
	Sel sel;
	Mv mv;
	bool ismv;
};

enum class SelDirection { LEFT, RIGHT, UP, DOWN };

// selects the topmost card at plc (or prepare to move there)
void selmv_byplace(Klon kln, SelMv& selmv, CardPlace plc);

// select another card at left, right, top or bottom, if possible
void selmv_anothercard(Klon kln, SelMv& selmv, SelDirection dir);

// if sel is in tableau and possible to select more/less cards in that tableau item, do that
// returns true if something was done, false otherwise
bool sel_more(Klon kln, Sel *sel);
bool sel_less(Klon kln, Sel *sel);

// sets selmv->ismv to true and updates other things correctly
void selmv_beginmv(SelMv& selmv);

// called when the user is done with dragging a card, moves the card if possible and resets sel
void selmv_endmv(Klon& kln, SelMv& selmv);

#endif  // SELMV_H
