#ifndef UI_H
#define UI_H

#include <curses.h>
#include "klon.hpp"  // IWYU pragma: keep
#include "selectmove.hpp" // IWYU pragma: keep

// sets up curses color pairs for SuitColor from card.h
void ui_initcolors();

// draws klon on win
// color and discardhide correspond to similarly named command-line arguments
void ui_drawklon(WINDOW *win, const Klondike& klon, const SelectionOrMove& selmv, bool color, bool discardhide);

#endif  // UI_H
