#include "help.h"
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "scroll.h"

static char *picture[] = {
	"╭──╮╭──╮    ╭──╮╭──╮╭──╮╭──╮",
	"│  ││  │    │ foundations  │",
	"╰──╯╰──╯    ╰──╯╰──╯╰──╯╰──╯",
	"  |   `--- discard          ",
	"  `--- stock                ",
	"╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮╭──╮",
	"│         tableau          │",
	"╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯╰──╯"
};

#define PICTURE_WIDTH (mbstowcs(NULL, picture[0], 0))
#define PICTURE_HEIGHT (sizeof(picture)/sizeof(picture[0]))

/*
in most functions, win can be NULL to not actually draw anything but count number of lines needed
w is width, if WINDOW is not NULL then w is what getmaxyx(win) gives
*/

static int get_max_width(int w, int xoff, int yoff)
{
	if (yoff > (int)PICTURE_HEIGHT)
		return w - xoff;
	return w - xoff - PICTURE_WIDTH - 3;  // 3 is for more space between picture and helps
}

static void maybe_mvwaddnstr(WINDOW *win, int y, int x, char *s, int n) { if (win) mvwaddnstr(win, y, x, s, n); }
static void maybe_mvwaddstr(WINDOW *win, int y, int x, char *s) { if (win) mvwaddstr(win, y, x, s); }

static void print_wrapped(WINDOW *win, int w, char *s, int xoff, int *yoff)
{
	int maxlen;
	while ((int)strlen(s) > (maxlen = get_max_width(w, xoff, *yoff))) {
		// strrchr doesn't work for this because the \0 isn't at maxlen
		// memrchr is a gnu extension :(
		int i = maxlen;
		while (s[i] != ' ' && i > 0)
			i--;

		if (i == 0) {
			// no spaces found, just break it at whatever is at maxlen
			maybe_mvwaddnstr(win, (*yoff)++, xoff, s, maxlen);
			s += maxlen;
		} else {
			maybe_mvwaddnstr(win, (*yoff)++, xoff, s, i);
			s += i+1;
		}
	}
	maybe_mvwaddstr(win, (*yoff)++, xoff, s);
}

static int get_longest_key_length(void)
{
	int max = 0, len;
	for (int i = 0; help[i].key && help[i].desc; i++)
		if ( (len = mbstowcs(NULL, help[i].key, 0)) > max )
			max = len;

	return max;
}

static void print_help_item(WINDOW *win, int w, struct Help help, int *y)
{
	int keymax = get_longest_key_length();
	int nspace = keymax - mbstowcs(NULL, help.key, 0);

	if (win)   // i didn't feel like creating a maybe_mvwprintw that messes with va_args
		mvwprintw(win, *y, 0, "%*s%s:", nspace, "", help.key);

	print_wrapped(win, w, help.desc, keymax + 2, y);
	//(*y)++;  // blank line
}

// returns number of lines
static int print_all_help(WINDOW *win, int w)
{
	int picx = w - PICTURE_WIDTH;

	for (int y = 0; y < (int)PICTURE_HEIGHT; y++)
		maybe_mvwaddstr(win, y, picx, picture[y]);

	int y = 0;
	for (int i = 0; help[i].key && help[i].desc; i++)
		print_help_item(win, w, help[i], &y);

#define max(a, b) ((a)>(b) ? (a) : (b))
	return max(y, (int)PICTURE_HEIGHT);
#undef max
}

void help_show(WINDOW *win)
{
	int w, h;
	getmaxyx(win, h, w);
	(void) h;  // silence warning about unused var

	WINDOW *pad = newpad(print_all_help(NULL, w), w);
	if (!pad)
		fatal_error("newpad() failed");
	print_all_help(pad, w);

	scroll_showpad(win, pad);
}
