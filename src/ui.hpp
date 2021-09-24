#ifndef UI_H
#define UI_H

#include <curses.h>
#include "args.hpp"
#include "klon.hpp"  // IWYU pragma: keep
#include "selectmove.hpp" // IWYU pragma: keep

// draws klon on win
// color and discardhide correspond to similarly named command-line arguments
void ui_draw(WINDOW *window, const Klondike& klon, const SelectionOrMove& selmv, const Args& args);

#endif  // UI_H
