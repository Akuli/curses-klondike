#ifndef UI_H
#define UI_H

#include <curses.h>
#include "card.h"

#define UI_CARDWIDTH 10
#define UI_CARDHEIGHT 7

void ui_drawcard(WINDOW *win, struct Card crd, int xcnt, int ycnt);

#endif  // UI_H
