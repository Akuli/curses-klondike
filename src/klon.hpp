// klon = klondike

#ifndef KLON_H
#define KLON_H

#include "card.hpp" // IWYU pragma: keep

// enumy values that represent places where cards can be moved to
typedef char KlonCardPlace;
#define KLON_STOCK 's'
#define KLON_DISCARD 'd'
#define KLON_FOUNDATION(n) ('F' + n)   // 0 <= n < 4
#define KLON_TABLEAU(n) ('T' + n)      // 0 <= n < 7

// because "val == KLON_FOUNDATION" doesn't do the right thing
#define KLON_IS_FOUNDATION(val) (KLON_FOUNDATION(0) <= (val) && (val) < KLON_FOUNDATION(4))
#define KLON_IS_TABLEAU(val)    (KLON_TABLEAU(0)    <= (val) && (val) < KLON_TABLEAU(7))

// KLON_FOUNDATION_NUM(KLON_FOUNDATION(n)) == n
#define KLON_FOUNDATION_NUM(fnd) (fnd - KLON_FOUNDATION(0))
#define KLON_TABLEAU_NUM(tab)    (tab - KLON_TABLEAU(0))

struct Klon {
	// https://www.denexa.com/wp-content/uploads/2015/11/klondike.png
	// these point to just one card, use ->next to access others
	Card *stock;           // TOPMOST card or NULL
	Card *discard;         // bottommost card or NULL
	unsigned int discardshow;     // number of cards shown in discard, or 0 if discard is NULL
	Card *foundations[4];  // bottommost cards or NULLs
	Card *tableau[7];      // bottommost cards or NULLs
	Card allcards[13*4];

	void debug_print() const;

	// not a part of constructor because unnecessary with e.g. dup() method
	void init();

	// copies everything from src to dst
	// also creates new cards
	// if srccrd is non-NULL, returns the corresponding card of dst
	// if srccrd is NULL, returns NULL
	Card *dup(Klon *dst, const Card *srccrd) const;

	// returns whether a card can be moved to on top of dst
	// use klon_stocktodiscard() instead for stock -> discard moves, this returns false for those
	// crd must be a card in kln
	bool canmove(const Card *crd, KlonCardPlace dst) const;

	// replaces crd with NULL
	// if crd is someothercrd->next, someothercrd is returned
	// if discard != NULL and crd == card_top(discard), updates discardshow
	Card *detachcard(const Card *crd);

	// moves the src card and ->next cards (if any) to dst
	// if raw, accepts invalid moves (canmove) and never sets ->visible
	void move(Card *crd, KlonCardPlace dst, bool raw);

	// convenience function for moving a card to any foundation
	// does nothing if card is NULL
	bool move2foundation(Card *card);

	// takes cards stock --> discard, or if stock is empty, puts all discardeds to stock
	// pick is the value of the --pick option
	void stock2discard(int pick);

	// check if the player has won
	bool win() const;
};

#endif  // KLON_H
