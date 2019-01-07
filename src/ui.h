#ifndef UI_H
#define UI_H

#include <curses.h>
#include <stdbool.h>
#include "card.h"   // IWYU pragma: keep
#include "klon.h"
#include "sel.h"

// sets up curses color pairs
void ui_initcolors(void);

// draws kln on win
// color and pick correspond to similarly named command-line arguments
// moving should be true if user is currently moving a card, otherwise false
void ui_drawklon(WINDOW *win, struct Klon kln, struct Sel sel, bool moving, bool color);

#endif  // UI_H
