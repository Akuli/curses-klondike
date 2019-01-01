#ifndef UI_H
#define UI_H

#include <curses.h>
#include "sol.h"

#define UI_CARDWIDTH 7
#define UI_CARDHEIGHT 5

// draws sol on win
void ui_drawsol(WINDOW *win, struct Sol sol);

#endif  // UI_H
