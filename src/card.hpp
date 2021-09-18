#ifndef CARD_H
#define CARD_H

#include <stdbool.h>

enum Suit { SUIT_SPADE, SUIT_HEART, SUIT_DIAMOND, SUIT_CLUB };

// must be >=1 because are used as curses color pairs
enum SuitColor { SUITCOLOR_RED = 1, SUITCOLOR_BLACK };

#define SUIT_COLOR(s) ( ((s)==SUIT_HEART || (s)==SUIT_DIAMOND) ? SUITCOLOR_RED : SUITCOLOR_BLACK )

struct Card {
	unsigned int num;  // 1 for A, 11 for J, 12 for Q, 13 for K, others the obvious way
	enum Suit suit;
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
