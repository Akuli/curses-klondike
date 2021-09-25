#ifndef SCROLL_H
#define SCROLL_H

#include <curses.h>

// see newpad() man page for more info about pads vs "normal" windows
void show_pad_with_scrolling(WINDOW *window, WINDOW *pad);

#endif
