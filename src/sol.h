#ifndef SOL_H
#define SOL_H

#include "card.h"   // IWYU pragma: keep

struct Sol {
	// https://www.denexa.com/wp-content/uploads/2015/11/klondike.png
	// these point to first cards, use ->next to access others
	struct Card *stock;
	struct Card *discard;
	struct Card *foundations[4];
	struct Card *tableau[7];
};

// list is the first card of a linked list from e.g. card_createallshuf()
void sol_init(struct Sol *sol, struct Card *list);

// prints debug info about where cards are
void sol_debug(struct Sol sol);

// frees all cards in the game
void sol_free(struct Sol sol);

#endif  // SOL_H
