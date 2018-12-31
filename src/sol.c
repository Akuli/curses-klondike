#include "sol.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.h"
#include "misc.h"

void sol_init(struct Sol *sol, struct Card *list)
{
	for (int i=0; i < 7; i++) {
		sol->tableau[i] = NULL;
		for (int j=0; j < i+1; j++)
			card_pushtop(&sol->tableau[i], card_popbot(&list));
		card_top(sol->tableau[i])->visible = true;
	}

	sol->stock = list;
	sol->discard = NULL;
	for (int i=0; i < 4; i++)
		sol->foundations[i] = NULL;
}

static int print_cards(struct Card *list)
{
	int n = 0;
	for (; list; list = list->next) {
		char s[CARD_STRMAX];
		card_str(*list, s);
		printf(" %s%s", list->visible ? "v" : "?", s);
		n++;
	}
	printf(" (%d cards)\n", n);
	return n;
}

void sol_debug(struct Sol sol)
{
	int total = 0;

	printf("stock:");
	total += print_cards(sol.stock);

	printf("discard:");
	total += print_cards(sol.discard);

	for (int i=0; i < 4; i++) {
		printf("foundation %d:", i);
		total += print_cards(sol.foundations[i]);
	}

	for (int i=0; i < 7; i++) {
		printf("tableau %d:", i);
		total += print_cards(sol.tableau[i]);
	}

	printf("total: %d cards\n", total);
}

void sol_free(struct Sol sol)
{
	card_free(sol.stock);
	card_free(sol.discard);
	for (int i=0; i < 4; i++)
		card_free(sol.foundations[i]);
	for (int i=0; i < 7; i++)
		card_free(sol.tableau[i]);
}

static void copy_cards(struct Card *src, struct Card **dst)
{
	*dst = NULL;
	struct Card *top = NULL;

	for (; src; src = src->next) {
		struct Card *dup = malloc(sizeof(struct Card));
		if (!dup)
			fatal_error("malloc() failed");

		memcpy(dup, src, sizeof(struct Card));
		dup->next = NULL;

		if (top)
			top->next = dup;
		else
			*dst = dup;
		top = dup;
	}
}

void sol_dup(struct Sol src, struct Sol *dst)
{
	copy_cards(src.stock, &dst->stock);
	copy_cards(src.discard, &dst->discard);
	for (int i=0; i < 4; i++)
		copy_cards(src.foundations[i], &dst->foundations[i]);
	for (int i=0; i < 7; i++)
		copy_cards(src.tableau[i], &dst->tableau[i]);
}
