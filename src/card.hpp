#ifndef CARD_H
#define CARD_H

#include <stdexcept>

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

private:
	Value val;
};

struct Card {
	unsigned int num;  // 1 for A, 11 for J, 12 for Q, 13 for K, others the obvious way
	Suit suit;
	bool visible;
	struct Card *next = nullptr;   // the card that is on top of this card
};

// creates a shuffled, linked list of 52 non-visible cards
// returns bottommost card, others are available with ->next
struct Card *card_createallshuf(void);

// frees the card, and its ->next and ->next->next etc
// does nothing if crd is NULL
void card_free(struct Card *crd);

// these return \0-terminated utf8
// don't free the result
const char *card_numstr(struct Card crd);
const char *card_suitstr(struct Card crd);

// prints card_str to stdout
void card_debug(struct Card crd);

// returns topmost card in a linked list of cards
// special case to make some things easier: card_top(NULL) == NULL
// card_top(nonnull)->next is always NULL
struct Card *card_top(struct Card *crd);

// returns at most n topmost cards from a linked list of cards as another linked list
// card_tops(NULL, n) == NULL
struct Card *card_tops(struct Card *crd, unsigned int n);

// gets bottommost card from a linked list of cards
// sets *bot to (*bot)->next (that can be NULL)
// bad things happen if *bot is NULL
struct Card *card_popbot(struct Card **bot);

// adds a card to top of a linked list of cards
// if *list is NULL, sets *list to newtop
// if *list is non-NULL, sets card_top(*list)->next to newtop
void card_pushtop(struct Card **list, struct Card *newtop);

#endif  // CARD_H
