#ifndef UI_H
#define UI_H

#include <curses.h>
#include "card.h"
#include "sol.h"

#define UI_CARDWIDTH 7
#define UI_CARDHEIGHT 5

// represents the card pile or location that the user has selected
// a pointer to struct Card is not enough because it's possible to select a
// place where cards can be put, but that currently contains no cards
struct UiSelection {
	struct Card *card;   // NULL for nothing selected
	SolCardPlace place;  // 0 for nothing selected
};

// draws sol on win
void ui_drawsol(WINDOW *win, struct Sol sol, struct UiSelection sel);

#endif  // UI_H
