#ifndef UI_H
#define UI_H

#include <curses.h>
#include "args.hpp"
#include "klon.hpp"  // IWYU pragma: keep
#include "selectmove.hpp" // IWYU pragma: keep

void ui_draw(WINDOW *window, const Klondike& klon, const SelectionOrMove& selmv, const Args& args);

#endif  // UI_H
