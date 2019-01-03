#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "card.h"
#include "misc.h"
#include "sol.h"
#include "ui.h"


static bool initscred = false;

// https://stackoverflow.com/a/8562768
// onerrorexit is externed in misc.h
static void exitcb(void)
{
	if (initscred) {
		endwin();
		initscred = false;
	}
}
void (*onerrorexit)(void) = exitcb;


// mv2 = move to
struct Game {
	struct Sol sol;
	struct UiSelection sel;  // the card moving or about to be moved
	SolCardPlace mv2;        // where to move the card, or 0 if user is not moving
};

static int place_2_card_x(SolCardPlace plc)
{
	if (SOL_IS_TABLEAU(plc))
		return SOL_TABLEAU_NUM(plc);
	if (SOL_IS_FOUNDATION(plc))
		return 3 + SOL_FOUNDATION_NUM(plc);
	if (plc == SOL_STOCK)
		return 0;
	if (plc == SOL_DISCARD)
		return 1;
	assert(0);
}

static SolCardPlace card_x_2_top_place(int x)
{
	if (x == 0)
		return SOL_STOCK;
	if (x == 1)
		return SOL_DISCARD;
	if (x == 2)
		return 0;

	assert(SOL_IS_FOUNDATION(SOL_FOUNDATION(x-3)));
	return SOL_FOUNDATION(x-3);
}

static struct Card *get_visible_top_card(struct Sol sol, SolCardPlace plc)
{
	if (SOL_IS_FOUNDATION(plc))
		return card_top(sol.foundations[SOL_FOUNDATION_NUM(plc)]);
	if (SOL_IS_TABLEAU(plc))
		return card_top(sol.tableau[SOL_TABLEAU_NUM(plc)]);
	if (plc == SOL_DISCARD)
		return card_top(sol.discard);
	return NULL;
}

// returns whether to continue playing
static bool handle_key(struct Game *gam, int k)
{
	int x = place_2_card_x(gam->mv2 ? gam->mv2 : gam->sel.place);
	bool tab = SOL_IS_TABLEAU(gam->mv2 ? gam->mv2 : gam->sel.place);

	// TODO: h help
	switch(k) {
	case 'q':
		return false;

	case 'd':
		sol_stock2discard(&gam->sol);
		break;

	case KEY_LEFT:
	case KEY_RIGHT:
		do
			x += (k == KEY_LEFT) ? -1 : 1;
		while( 0 <= x && x < 7 && !tab && card_x_2_top_place(x) == 0 );

		if (0 <= x && x < 7) {
			if (gam->mv2)
				gam->mv2 = tab ? SOL_TABLEAU(x) : card_x_2_top_place(x);
			else {
				gam->sel.place = tab ? SOL_TABLEAU(x) : card_x_2_top_place(x);
				gam->sel.card = get_visible_top_card(gam->sol, gam->sel.place);
			}
		}

		break;

	case KEY_UP:
		if (gam->mv2) {
			if (tab) {
				// can't move multiple cards to foundations
				if (SOL_IS_FOUNDATION(card_x_2_top_place(x)) && !gam->sel.card->next)
					gam->mv2 = card_x_2_top_place(x);
			} else
				gam->mv2 = SOL_TABLEAU(x);
		} else if (tab) {
			// can select more cards?
			bool selmr = false;
			for (struct Card *crd = gam->sol.tableau[x]; crd && crd->next; crd = crd->next) {
				if (gam->sel.card != crd->next || !crd->visible)
					continue;
				gam->sel.card = crd;
				selmr = true;
				break;
			}

			// if not, move selection to to top row
			if (!selmr && card_x_2_top_place(x) != 0) {
				gam->sel.place = card_x_2_top_place(x);
				gam->sel.card = get_visible_top_card(gam->sol, gam->sel.place);
			}
		}
		break;

	case KEY_DOWN:
		if (gam->mv2)
			gam->mv2 = SOL_TABLEAU(x);
		else {
			if (tab && gam->sel.card && gam->sel.card->next)
				gam->sel.card = gam->sel.card->next;
			if (!tab) {
				gam->sel.place = SOL_TABLEAU(x);
				gam->sel.card = get_visible_top_card(gam->sol, gam->sel.place);
			}
		}
		break;

	case '\n':
		// TODO: allow moving cards out of foundations one by one
		if (gam->mv2) {
			assert(gam->sel.card);
			if (sol_canmove(gam->sol, gam->sel.card, gam->mv2)) {
				sol_move(&gam->sol, gam->sel.card, gam->mv2);
				gam->sel.place = gam->mv2;
				gam->sel.card = get_visible_top_card(gam->sol, gam->sel.place);
			} else {
				gam->sel.place = gam->mv2;
				gam->sel.card = get_visible_top_card(gam->sol, gam->sel.place);
			}
			gam->mv2 = 0;
		}
		else if (gam->sel.place == SOL_STOCK)
			sol_stock2discard(&gam->sol);
		else if (  // FIXME: only the first of the 3 cases works! other cases leave gam->sel.card to NULL
				(gam->sel.card && gam->sel.card->visible && !gam->mv2) ||
				SOL_IS_FOUNDATION(gam->sel.place) ||
				gam->sel.place == SOL_DISCARD) {
			assert(gam->sel.card);
			gam->mv2 = gam->sel.place;
		}
		break;

	default:
		break;
	}

	return true;
}

int main(void)
{
	// displaying unicodes correctly needs setlocale here AND cursesw instead of curses in makefile
	if (!setlocale(LC_ALL, ""))
		fatal_error("setlocale() failed");

	time_t t = time(NULL);
	if (t == (time_t)(-1))
		fatal_error("time() failed");
	srand(t);

	if (!initscr())
		fatal_error("initscr() failed");
	initscred = true;

	if (cbreak() == ERR)
		fatal_error("cbreak() failed");
	if (curs_set(0) == ERR)
		fatal_error("curs_set() failed");
	if (keypad(stdscr, true) == ERR)
		fatal_error("keypad() failed");

	refresh();   // yes, this is needed before drawing the cards for some reason

	struct Game gam;
	sol_init(&gam.sol, card_createallshuf());
	gam.sel.card = NULL;
	gam.sel.place = SOL_STOCK;
	gam.mv2 = 0;

	do {
		if (gam.mv2) {
			// create a temp copy of the sol for rendering
			struct Sol tmpsol;
			struct UiSelection tmpsel;
			tmpsel.card = sol_dup(gam.sol, &tmpsol, gam.sel.card);

			sol_rawmove(&tmpsol, tmpsel.card, gam.mv2);
			tmpsel.place = gam.mv2;

			ui_drawsol(stdscr, tmpsol, tmpsel);
			sol_free(tmpsol);
		} else
			ui_drawsol(stdscr, gam.sol, gam.sel);
		refresh();
	} while( handle_key(&gam, getch()) );

	sol_free(gam.sol);
	endwin();

	return 0;
}
