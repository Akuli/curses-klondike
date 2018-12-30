#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include "misc.h"


static bool initscred = false;

// https://stackoverflow.com/a/8562768
static void exitcb(void)
{
	if (initscred) {
		endwin();
		initscred = false;
	}
}
void (*onerrorexit)(void) = exitcb;


int main(void)
{
	// displaying unicodes correctly needs setlocale here AND -lncursesw in makefile
	if (!setlocale(LC_ALL, ""))
		fatal_error("setlocale() failed");

	initscr();
	initscred = true;

	printw("asd asd \xe2\x99\xa0");
	refresh();
	getch();
	endwin();

	return 0;
}
