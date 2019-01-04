#include "help.h"
#include <curses.h>
#include <stdlib.h>
#include <string.h>

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


static int get_max_width(WINDOW *win, int xoff, int yoff)
{
	if (yoff > (int)PICTURE_HEIGHT)
		return getmaxx(win) - xoff;
	return getmaxx(win) - xoff - PICTURE_WIDTH - 3;  // 3 is for more space between picture and helps
}

// returns number of lines printed
static void print_wrapped(WINDOW *win, char *s, int xoff, int *yoff)
{
	int maxlen;
	while ((int)strlen(s) > (maxlen = get_max_width(win, xoff, *yoff))) {
		// strrchr doesn't work for this because the \0 isn't at maxlen
		// memrchr is a gnu extension :(
		int i = maxlen;
		while (s[i] != ' ' && i > 0)
			i--;

		if (i == 0) {
			// no spaces found, just break it at whatever is at maxlen
			mvwaddnstr(win, (*yoff)++, xoff, s, maxlen);
			s += maxlen;
		} else {
			mvwaddnstr(win, (*yoff)++, xoff, s, i);
			s += i+1;
		}
	}
	mvwaddstr(win, (*yoff)++, xoff, s);
}

static int get_longest_key_length(void)
{
	int max = 0, len;
	for (int i = 0; help[i].key && help[i].desc; i++)
		if ( (len = mbstowcs(NULL, help[i].key, 0)) > max )
			max = len;

	return max;
}

static void print_help_item(WINDOW *win, struct Help help, int *y)
{
	int keymax = get_longest_key_length();
	int nspace = keymax - mbstowcs(NULL, help.key, 0);
	mvwprintw(win, *y, 0, "%*s%s:", nspace, "", help.key);

	print_wrapped(win, help.desc, keymax + 2, y);
	//(*y)++;  // blank line
}

void help_show(WINDOW *win)
{
	do {
		werase(win);
		int wdth = getmaxx(win);
		int picx = wdth - PICTURE_WIDTH;
		for (int y = 0; y < (int)PICTURE_HEIGHT; y++)
			mvwaddstr(win, y, picx, picture[y]);

		int y = 0;
		for (int i = 0; help[i].key && help[i].desc; i++)
			print_help_item(win, help[i], &y);

		y++;
		print_wrapped(win, "Press any key to get out of this help...", 0, &y);
	} while (getch() == KEY_RESIZE);
}
