#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "card.h"
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

	time_t t = time(NULL);
	if (t == (time_t)(-1))
		fatal_error("time() failed");
	srand(t);

	struct Card *botcrd = card_createallshuf();

	for (struct Card *crd = botcrd; crd; crd = crd->next)
		printf("%d %d\n", (int)crd->suit, crd->num);

	card_free(botcrd);

	/*
	initscr();
	initscred = true;

	printw("asd asd \xe2\x99\xa0");
	refresh();
	getch();
	endwin();
	*/

	return 0;
}
