// klon = klondike

#ifndef KLON_H
#define KLON_H

#include "card.hpp" // IWYU pragma: keep
#include <array>
#include <cassert>
#include <cstdint>

struct CardPlace {
	enum Kind : uint8_t { STOCK, DISCARD, FOUNDATION, TABLEAU };

	Kind kind;
	int8_t number;

	static CardPlace stock() { return CardPlace{ STOCK, -1 }; }
	static CardPlace discard() { return CardPlace{ DISCARD, -1 }; }
	static CardPlace foundation(int n) { assert(0 <= n && n < 4); return CardPlace{ FOUNDATION, (int8_t)n }; }
	static std::array<CardPlace, 4> foundations() { return { foundation(0), foundation(1), foundation(2), foundation(3) }; }
	static CardPlace tableau(int n) { assert(0 <= n && n < 7); return CardPlace{ TABLEAU, (int8_t)n }; }

	bool operator==(CardPlace other) const { return this->kind == other.kind && this->number == other.number; }
	bool operator!=(CardPlace other) const { return !(*this == other); }
};

struct Klondike {
	// https://www.denexa.com/wp-content/uploads/2015/11/klondike.png
	// these point to just one card, use ->next to access others
	Card *stock;                       // TOPMOST card or nullptr
	Card *discard;                     // bottommost card or nullptr
	int discardshow;                   // number of cards shown in discard, or 0 if discard is nullptr
	std::array<Card*, 4> foundations;  // bottommost cards or nullptrs
	std::array<Card*, 7> tableau;      // bottommost cards or nullptrs

	// not a part of constructor because unnecessary with e.g. dup() method
	void init(std::array<Card, 13*4>& card_array);

	// copies everything from src to dest, using new cards created from the array
	// if source_card is non-nullptr, returns the corresponding card of dest
	// if source_card is nullptr, returns nullptr
	Card *dup(Klondike& dest, const Card *source_card, std::array<Card, 13*4>& card_array) const;

	// returns whether a card can be moved to on top of dest
	// use stock2discard() instead for stock -> discard moves, this returns false for those
	// card must be a card in klon
	bool can_move(const Card *card, CardPlace dest) const;

	// replaces card with nullptr
	// if card is someothercard->next, someothercard is returned
	// if discard != nullptr and card == top(discard), updates discardshow
	Card *detach_card(const Card *card);

	// moves the source card and ->next cards (if any) to dest
	// if raw, accepts invalid moves (can_move) and never sets ->visible
	void move(Card *card, CardPlace dest, bool raw);

	// move card to any foundation
	// does nothing if card is nullptr
	bool move2foundation(Card *card);

	// takes cards stock --> discard, or if stock is empty, puts all discardeds to stock
	// pick is the value of the --pick option
	void stock2discard(int pick);

	// check if the player has won
	bool win() const;
};

#endif  // KLON_H
