#include "scroll.hpp"
#include <algorithm>
#include <curses.h>
#include <climits>

static constexpr int BOTTOM_BAR_SIZE = 1;

struct Scroller {
	WINDOW *const win;
	WINDOW *const pad;
	int firstlineno = 0;

	// makes sure that it's not scrolled too far up or down
	void bounds_check()
	{
		int winh, padh, w;
		getmaxyx(this->win, winh, w);
		getmaxyx(this->pad, padh, w);
		winh -= BOTTOM_BAR_SIZE;
		(void) w;  // w is needed only because getmaxyx wants it, this suppresses warning

		// it's important that the negativeness check is last
		// this way firstlineno is set to 0 if padh < winh
		if (this->firstlineno > padh - winh)
			this->firstlineno = padh - winh;
		if (this->firstlineno < 0)
			this->firstlineno = 0;
	}

	void draw_pad_to_window()
	{
		this->bounds_check();

		int winw, winh, padw, padh;
		getmaxyx(this->win, winh, winw);
		getmaxyx(this->pad, padh, padw);
		winh -= BOTTOM_BAR_SIZE;

		wclear(this->win);  // werase() doesn't fill window with dark background in color mode

		// min stuff and -1 are needed because curses is awesome
		// if this code is wrong, it either segfaults or does nothing
		copywin(this->pad, this->win, this->firstlineno, 0, 0, 0, std::min(winh, padh)-1, std::min(winw, padw)-1, true);

		const char *msg;
		if (winh < padh)
			msg = "Move with ↑ and ↓, or press q to quit.";
		else
			msg = "Press q to quit.";

		wattron(this->win, A_STANDOUT);
		mvwaddstr(this->win, winh, 0, msg);
		wattroff(this->win, A_STANDOUT);

		wrefresh(this->win);
	}

	void handle_key(int k)
	{
		int winw, winh;
		getmaxyx(this->win, winh, winw);
		winh -= BOTTOM_BAR_SIZE;
		(void) winw;

		switch(k) {
			case KEY_UP: case 'p': this->firstlineno--; break;
			case KEY_DOWN: case 'n': this->firstlineno++; break;
			case KEY_PPAGE: this->firstlineno -= winh; break;
			case KEY_NPAGE: case ' ': this->firstlineno += winh; break;
			case KEY_HOME: this->firstlineno = 0; break;
			case KEY_END: this->firstlineno = INT_MAX; break;
			default: break;
		}
	}
};

// must wrefresh(win) after this, but not before
void scroll_showpad(WINDOW *win, WINDOW *pad)
{
	Scroller s = { win, pad };

	while(1) {
		werase(win);
		s.draw_pad_to_window();
		wrefresh(win);

		int k = getch();
		if (k == 'q')
			break;
		s.handle_key(k);
	}
}
