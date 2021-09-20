// klon = klondike

#ifndef KLON_H
#define KLON_H

#include "card.hpp" // IWYU pragma: keep
#include <cassert>

struct CardPlace {
	enum Kind : uint8_t { STOCK, DISCARD, FOUNDATION, TABLEAU };

	Kind kind;
	int8_t num;

	static CardPlace stock() { return CardPlace{ STOCK, -1 }; }
	static CardPlace discard() { return CardPlace{ DISCARD, -1 }; }
	static CardPlace foundation(int n) { assert(0 <= n && n < 4); return CardPlace{ FOUNDATION, (int8_t)n }; }
	static CardPlace tableau(int n) { assert(0 <= n && n < 7); return CardPlace{ TABLEAU, (int8_t)n }; }

	bool operator==(CardPlace other) {
		return this->kind == other.kind && this->num == other.num;
	}
};

struct Klon {
	// https://www.denexa.com/wp-content/uploads/2015/11/klondike.png
	// these point to just one card, use ->next to access others
	Card *stock;           // TOPMOST card or NULL
	Card *discard;         // bottommost card or NULL
	int discardshow;       // number of cards shown in discard, or 0 if discard is NULL
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
	Card *dup(Klon& dst, const Card *srccrd) const;

	// returns whether a card can be moved to on top of dst
	// use klon_stocktodiscard() instead for stock -> discard moves, this returns false for those
	// crd must be a card in kln
	bool canmove(const Card *crd, CardPlace dst) const;

	// replaces crd with NULL
	// if crd is someothercrd->next, someothercrd is returned
	// if discard != NULL and crd == card_top(discard), updates discardshow
	Card *detachcard(const Card *crd);

	// moves the src card and ->next cards (if any) to dst
	// if raw, accepts invalid moves (canmove) and never sets ->visible
	void move(Card *crd, CardPlace dst, bool raw);

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
