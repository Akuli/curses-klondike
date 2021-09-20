#ifndef HELP_H
#define HELP_H

#include <curses.h>
#include <cassert>
#include <vector>
#include <string>

// wstrings because ♥ is one character, not 3 characters
static_assert(sizeof(wchar_t) >= 4);

struct HelpKey {
	std::wstring key;
	std::wstring desc;
};

// screen must be erased after calling, but not before
void help_show(WINDOW *win, std::vector<HelpKey> hkeys, const char *argv0, bool color);

#endif  // HELP_H
