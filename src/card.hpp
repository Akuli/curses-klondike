#ifndef CARD_H
#define CARD_H

#include <stdexcept>
#include <string>

// https://stackoverflow.com/a/53284026

class SuitColor {
public:
	enum Value { RED, BLACK };
	SuitColor() = default;
	constexpr SuitColor(Value v) : val(v) {}
	operator Value() const { return val; }

	int color_pair_number() { return static_cast<int>(val) + 1; }  // must be >=1

private:
	Value val;
};

class Suit {
public:
	enum Value { SPADE, HEART, DIAMOND, CLUB };
	Suit() = default;
	constexpr Suit(Value v) : val(v) {}
	operator Value() const { return val; }

	SuitColor color() const {
		switch(*this) {
			case Suit::HEART:
			case Suit::DIAMOND:
				return SuitColor::RED;
			case Suit::SPADE:
			case Suit::CLUB:
				return SuitColor::BLACK;
		}
		throw std::logic_error("bad enum value");
	}

	std::string string() const {
		switch(*this) {
			case Suit::SPADE: return "♠";
			case Suit::HEART: return "♥";
			case Suit::DIAMOND: return "♦";
			case Suit::CLUB: return "♣";
		}
		throw std::logic_error("bad suit");
	}

private:
	Value val;
};

struct Card {
	int num;  // 1 for A, 11 for J, 12 for Q, 13 for K, others the obvious way
	Suit suit;
	bool visible = false;
	Card *next = nullptr;   // the card that is on top of this card

	std::string numstring() const;  // e.g. "A"
	void debug_print() const;
};

namespace cardlist {
	// initialize and shuffle an array of cards, and build a linked list of them
	Card *init(Card (&arr)[13*4]);

	// returns topmost card in a linked list of cards
	// special case to make some things easier: top(nullptr) == nullptr
	// top(nonnull)->next is always nullptr
	Card *top(Card *crd);

	// returns at most n topmost cards from a linked list of cards as another linked list
	// tops(nullptr, n) == nullptr
	Card *tops(Card *crd, int n);

	// gets bottommost card from a linked list of cards
	// sets *bot to (*bot)->next (that can be nullptr)
	// bad things happen if *bot is nullptr
	Card *pop_bottom(Card **bot);

	// adds a card to top of a linked list of cards
	// if *list is nullptr, sets *list to newtop
	// if *list is non-nullptr, sets top(*list)->next to newtop
	void push_top(Card **list, Card *newtop);
}

#endif  // CARD_H
