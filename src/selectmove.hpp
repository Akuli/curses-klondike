#ifndef SELECTMOVE_H
#define SELECTMOVE_H

#include "card.hpp"  // IWYU pragma: keep
#include "klondike.hpp"

enum class SelDirection { LEFT, RIGHT, UP, DOWN };

/*
represents the card pile or location that the user has selected
a pointer to Card is not enough because it's possible to select a
place where cards can be put but currently contains no cards

possible values:

	.card = nullptr, .place = KLON_STOCK
		stock selected

	.card = cardlist::top(klon.discard), .place = CardPlace::discard()
		discard selected

	.card = cardlist::top(klon.foundations[n]), .place = CardPlace::foundation(n)
		nth foundation selected

	.card = klon.tableau[n] or some of its ->nexts, .card is visible, .place = CardPlace::tableau(n)
		nth tableau selected, including the specified card and all its ->nexts

	.card = nullptr, .place = CardPlace::tableau(n)
		tableau n selected, but there are no cards in that tableau
*/
struct Selection {
	Card *card;
	CardPlace place;

	// return true if something was done, false otherwise
	bool select_more(const Klondike& klon);
	bool select_less(const Klondike& klon);
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

	void select_top_card_or_move_to(const Klondike& klon, CardPlace place);
	void select_another_card(const Klondike& klon, SelDirection dir);
	void begin_move();
	void end_move(Klondike& klon);  // moves card if possible
};


#endif  // SELECTMOVE_H
