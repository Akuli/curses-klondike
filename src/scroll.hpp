#ifndef SCROLL_H
#define SCROLL_H

#include <curses.h>

// show a curses pad window so that the user can scroll up and down with arrow keys
// see newpad() man page for more info about pads vs "normal" windows
void scroll_showpad(WINDOW *window, WINDOW *pad);

#endif
