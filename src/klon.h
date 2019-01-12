// klon = klondike

#ifndef KLON_H
#define KLON_H

#include <stdbool.h>
#include "card.h"   // IWYU pragma: keep

struct Klon {
	// https://www.denexa.com/wp-content/uploads/2015/11/klondike.png
	// these point to just one card, use ->next to access others
	struct Card *stock;           // TOPMOST card or NULL
	struct Card *discard;         // bottommost card or NULL
	unsigned int discardshow;     // number of cards shown in discard, or 0 if discard is NULL
	struct Card *foundations[4];  // bottommost cards or NULLs
	struct Card *tableau[7];      // bottommost cards or NULLs
};

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

// list is the first card of a linked list from e.g. card_createallshuf()
void klon_init(struct Klon *kln, struct Card *list);

// prints debug info about where cards are
void klon_debug(struct Klon kln);

// frees all cards in the game
void klon_free(struct Klon kln);

// copies everything from src to dst
// also creates new cards
// if srccrd is non-NULL, returns the corresponding card of dst
// if srccrd is NULL, returns NULL
struct Card *klon_dup(struct Klon src, struct Klon *dst, const struct Card *srccrd);

// returns whether a card can be moved to on top of dst
// use klon_stocktodiscard() instead for stock -> discard moves, this returns false for those
// crd must be a card in kln
bool klon_canmove(struct Klon kln, const struct Card *crd, KlonCardPlace dst);

// replaces crd with NULL in kln
// if crd is someothercrd->next, someothercrd is returned
// if kln->discard != NULL and crd == card_top(kln->discard), updates kln->discardshow
struct Card *klon_detachcard(struct Klon *kln, const struct Card *crd);

// moves the src card and ->next cards (if any) to dst
// the move must be valid, see klon_canmove()
// if raw, accepts invalid moves (klon_canmove) and never sets ->visible
void klon_move(struct Klon *kln, struct Card *crd, KlonCardPlace dst, bool raw);

// takes cards stock --> discard, or if stock is empty, puts all discardeds to stock
// pick is the value of the --pick option
void klon_stock2discard(struct Klon *kln, unsigned int pick);

// check if the player has won
bool klon_win(struct Klon kln);

#endif  // KLON_H
