#ifndef UI_H
#define UI_H

#include <curses.h>
#include "card.h"

#define UI_CARDWIDTH 10
#define UI_CARDHEIGHT 7

// draws crd on win
// xo, yob, yos are for drawing cards so that they partially overlap
// xo = x offset, yos = small y offset, yob = guess what
void ui_drawcard(WINDOW *win, struct Card crd, int xcnt, int ycnt, int xo, int yos, int yob);

#endif  // UI_H
