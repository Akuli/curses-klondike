#ifndef CORE_H
#define CORE_H

#include <stdbool.h>

enum Suit { SUIT_SPADE, SUIT_HEART, SUIT_DIAMOND, SUIT_CLUB };

// an array of all suits
extern enum Suit suit_all[4];

struct Card {
	unsigned int num;
	enum Suit suit;
	bool visible;
	struct Card *next;   // the card that is on top of this card, or NULL
};

// creates a shuffled, linked list of 52 non-visible cards
// returns bottommost card, others are available with ->next
struct Card *card_createallshuf(void);

// frees the card, and its ->next and ->next->next etc
void card_free(struct Card *crd);

#endif  // CORE_H
