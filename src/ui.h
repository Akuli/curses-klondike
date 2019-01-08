#ifndef UI_H
#define UI_H

#include <curses.h>
#include <stdbool.h>
#include "card.h"   // IWYU pragma: keep
#include "klon.h"
#include "selmv.h"

// sets up curses color pairs
void ui_initcolors(void);

// draws kln on win
// color corresponds to a similarly named command-line argument
void ui_drawklon(WINDOW *win, struct Klon kln, struct SelMv selmv, bool color);

#endif  // UI_H
