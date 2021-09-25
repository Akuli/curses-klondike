#ifndef HELP_H
#define HELP_H

#include <cursesw.h>
#include <string_view>
#include <vector>

struct HelpItem {
	std::string_view key;
	std::string_view desc;
};

// screen must be erased after calling this
void show_help(WINDOW *win, std::vector<HelpItem> hkeys, const char *argv0, bool color);

#endif  // HELP_H
