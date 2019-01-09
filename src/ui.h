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
// color and discardhide correspond to similarly named command-line arguments
void ui_drawklon(WINDOW *win, struct Klon kln, struct SelMv selmv, bool color, bool discardhide);

#endif  // UI_H
