#ifndef SELMV_H
#define SELMV_H

#include "card.hpp"  // IWYU pragma: keep
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

	.card = top(klon->discard), .place = KLON_DISCARD
		discard selected

	.card = top(klon->foundations[n]), .place = KLON_FOUNDATION(n)
		nth foundation selected

	.card = klon->tableau[n] or some of its ->nexts, .card is visible, .place = KLON_TABLEAU(n)
		nth tableau selected, including the specified card and all its ->nexts

	.card = nullptr, .place = KLON_TABLEAU(n)
		tableau n selected, but there are no cards in that tableau
*/
struct Selection {
	Card *card;
	CardPlace place;

	// if sel is in tableau and possible to select more/less cards in that tableau item, do that
	// returns true if something was done, false otherwise
	bool more(const Klondike& klon);
	bool less(const Klondike& klon);
};

// represents card being moved src --> dest
struct Move {
	Card *card;
	CardPlace src;
	CardPlace dest;
};

struct SelectionOrMove {
	Selection sel;
	Move move;
	bool ismove;

	void select_top_card_at_place(const Klondike& klon, CardPlace plc);
	void select_another_card(const Klondike& klon, SelDirection dir);
	void begin_move();
	void end_move(Klondike& klon);  // moves card if possible
};


#endif  // SELMV_H
