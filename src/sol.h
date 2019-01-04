#ifndef SOL_H
#define SOL_H

#include <stdbool.h>
#include "card.h"   // IWYU pragma: keep

struct Sol {
	// https://www.denexa.com/wp-content/uploads/2015/11/klondike.png
	// these point to just one card, use ->next to access others
	struct Card *stock;           // TOPMOST card or NULL
	struct Card *discard;         // bottommost card or NULL
	struct Card *foundations[4];  // bottommost cards or NULLs
	struct Card *tableau[7];      // bottommost cards or NULLs
};

// enumy values that represent places where cards can be moved to
typedef char SolCardPlace;
#define SOL_STOCK 's'
#define SOL_DISCARD 'd'
#define SOL_FOUNDATION(n) ('F' + n)   // 0 <= n < 4
#define SOL_TABLEAU(n) ('T' + n)      // 0 <= n < 7

// because "val == SOL_FOUNDATION" doesn't do the right thing
#define SOL_IS_FOUNDATION(val) (SOL_FOUNDATION(0) <= (val) && (val) < SOL_FOUNDATION(4))
#define SOL_IS_TABLEAU(val)    (SOL_TABLEAU(0)    <= (val) && (val) < SOL_TABLEAU(7))

// SOL_FOUNDATION_NUM(SOL_FOUNDATION(n)) == n
#define SOL_FOUNDATION_NUM(fnd) (fnd - SOL_FOUNDATION(0))
#define SOL_TABLEAU_NUM(tab)    (tab - SOL_TABLEAU(0))

// list is the first card of a linked list from e.g. card_createallshuf()
void sol_init(struct Sol *sol, struct Card *list);

// prints debug info about where cards are
void sol_debug(struct Sol sol);

// frees all cards in the game
void sol_free(struct Sol sol);

// copies everything from src to dst
// also creates new cards
// if srccrd is non-NULL, returns the corresponding card of dst
// if srccrd is NULL, returns NULL
struct Card *sol_dup(struct Sol src, struct Sol *dst, struct Card *srccrd);

// returns whether a card can be moved to on top of dst
// use sol_stocktodiscard() instead for stock -> discard moves, this returns false for those
// crd must be a card in sol
bool sol_canmove(struct Sol sol, struct Card *crd, SolCardPlace dst);

// replaces crd with NULL in sol
// if crd is someothercrd->next, someothercrd is returned
struct Card *sol_detachcard(struct Sol *sol, struct Card *crd);

// moves the src card and ->next cards (if any) to dst
// the move must be valid, see sol_canmove()
void sol_move(struct Sol *sol, struct Card *crd, SolCardPlace dst);

// similar to sol_move, but accepts invalid moves (sol_canmove) and never sets ->visible
void sol_rawmove(struct Sol *sol, struct Card *crd, SolCardPlace dst);

// moves crd to a foundation, if possible
void sol_2foundation(struct Sol *sol, struct Card *crd);

// takes a card stock --> discard, or if stock is empty, puts all discardeds to stock
void sol_stock2discard(struct Sol *sol);

// check if the player has won
bool sol_win(struct Sol sol);

#endif  // SOL_H
