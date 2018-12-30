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

// writes a \0-terminated utf8 string representiation of the card to buf
// useful for ui and debugging
// buf must have room for at least CARD_STRMAX bytes (including \0)
void card_str(struct Card crd, char *buf);

// returns topmost card in a linked list of cards
// bad things happen if crd is NULL
// never returns NULL
// card_top(...)->next is always NULL
struct Card *card_top(struct Card *crd);

// for handling linked lists of cards
// moves bottommost card of *srcbot to on top of card_top(dst), handling ->next properly
// bad things happen if *srcbot is NULL
// if (*srcbot)->next is NULL, *srcbot is set to NULL
void card_movebot2top(struct Card **srcbot, struct Card *dst);

// Card.num representations: A,2,3,...,9,10,J,Q,K (longest has 2 chars)
// suit is 3 bytes of utf8
// this includes a trailing \0
#define CARD_STRMAX (2+3+1)

#endif  // CORE_H
