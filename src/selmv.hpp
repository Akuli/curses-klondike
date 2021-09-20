#ifndef SELMV_H
#define SELMV_H

#include <stdbool.h>
#include "klon.hpp"

enum class SelDirection { LEFT, RIGHT, UP, DOWN };

/*
represents the card pile or location that the user has selected
a pointer to Card is not enough because it's possible to select a
place where cards can be put but currently contains no cards

possible values:

	.card = nullptr, .place = 0
		nothing selected

	.card = nullptr, .place = KLON_STOCK
		stock selected

	.card = card_top(kln->discard), .place = KLON_DISCARD
		discard selected

	.card = card_top(kln->foundations[n]), .place = KLON_FOUNDATION(n)
		nth foundation selected

	.card = kln->tableau[n] or some of its ->nexts, .card is visible, .place = KLON_TABLEAU(n)
		nth tableau selected, including the specified card and all its ->nexts

	.card = nullptr, .place = KLON_TABLEAU(n)
		tableau n selected, but there are no cards in that tableau
*/
struct Sel {
	Card *card;
	CardPlace place;

	// if sel is in tableau and possible to select more/less cards in that tableau item, do that
	// returns true if something was done, false otherwise
	bool more(const Klon& kln);
	bool less(const Klon& kln);
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

	void select_top_card_at_place(const Klon& kln, CardPlace plc);
	void select_another_card(const Klon& kln, SelDirection dir);
	void begin_move();
	void end_move(Klon& kln);  // moves card if possible
};


#endif  // SELMV_H
