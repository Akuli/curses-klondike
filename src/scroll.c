#include "scroll.hpp"
#include <algorithm>
#include <curses.h>
#include <limits>

static constexpr int BOTTOM_BAR_SIZE = 1;

struct Scroller {
	WINDOW *const window;
	WINDOW *const pad;
	int firstlineno = 0;

	// makes sure that it's not scrolled too far up or down
	void bounds_check()
	{
		int window_height, pad_height, w;
		getmaxyx(this->window, window_height, w);
		getmaxyx(this->pad, pad_height, w);
		window_height -= BOTTOM_BAR_SIZE;
		(void) w;  // w is needed only because getmaxyx wants it, this suppresses warning

		// it's important that the negativeness check is last
		// this way firstlineno is set to 0 if pad_height < window_height
		if (this->firstlineno > pad_height - window_height)
			this->firstlineno = pad_height - window_height;
		if (this->firstlineno < 0)
			this->firstlineno = 0;
	}

	void draw_pad_to_window()
	{
		this->bounds_check();

		int window_width, window_height, pad_width, pad_height;
		getmaxyx(this->window, window_height, window_width);
		getmaxyx(this->pad, pad_height, pad_width);
		window_height -= BOTTOM_BAR_SIZE;

		wclear(this->window);  // werase() doesn't fill window with dark background in color mode

		// min stuff and -1 are needed because curses is awesome
		// if this code is wrong, it either segfaults or does nothing
		copywin(this->pad, this->window, this->firstlineno, 0, 0, 0, std::min(window_height, pad_height)-1, std::min(window_width, pad_width)-1, true);

		const char *msg;
		if (window_height < pad_height)
			msg = "Move with ↑ and ↓, or press q to quit.";
		else
			msg = "Press q to quit.";

		wattron(this->window, A_STANDOUT);
		mvwaddstr(this->window, window_height, 0, msg);
		wattroff(this->window, A_STANDOUT);

		wrefresh(this->window);
	}

	void handle_key(int key)
	{
		int window_width, window_height;
		getmaxyx(this->window, window_height, window_width);
		window_height -= BOTTOM_BAR_SIZE;
		(void) window_width;

		switch(key) {
			case KEY_UP: case 'p': this->firstlineno--; break;
			case KEY_DOWN: case 'n': this->firstlineno++; break;
			case KEY_PPAGE: this->firstlineno -= window_height; break;
			case KEY_NPAGE: case ' ': this->firstlineno += window_height; break;
			case KEY_HOME: this->firstlineno = 0; break;
			case KEY_END: this->firstlineno = std::numeric_limits<int>::max(); break;
			default: break;
		}
	}
};

void show_pad_with_scrolling(WINDOW *window, WINDOW *pad)
{
	Scroller scroller = { window, pad };
	while(1) {
		werase(window);
		scroller.draw_pad_to_window();
		wrefresh(window);

		int key = getch();
		if (key == 'q')
			break;
		scroller.handle_key(key);
	}
}
