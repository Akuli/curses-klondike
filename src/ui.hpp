#ifndef UI_H
#define UI_H

#include <curses.h>
#include <stdbool.h>
#include "klon.hpp"  // IWYU pragma: keep
#include "selmv.hpp" // IWYU pragma: keep

// sets up curses color pairs for SuitColor from card.h
void ui_initcolors(void);

// draws kln on win
// color and discardhide correspond to similarly named command-line arguments
void ui_drawklon(WINDOW *win, Klon kln, SelMv selmv, bool color, bool discardhide);

#endif  // UI_H
