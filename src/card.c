#include "card.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

// handy_random(n) returns a random integer >= 0 and < n
static unsigned int handy_random(unsigned int n)
{
	// have fun figuring out how this works
	int toobig = (RAND_MAX / n) * n;
	int res;
	do {
		res = rand();
	} while (res >= toobig);
	return res % n;
}

// https://stackoverflow.com/a/6274381
static void shuffle(struct Card **arr, int len)
{
	for (int i = len-1; i > 0; i--) {
		int j = handy_random(i+1);
		struct Card *tmp = arr[i];
		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

struct Card *card_createallshuf(void)
{
	struct Card *cards[13*4 + 1];
	cards[13*4] = NULL;
	enum Suit suits[] = { SUIT_SPADE, SUIT_HEART, SUIT_DIAMOND, SUIT_CLUB };

	int i = 0;
	for (int s = 0; s < 4; s++) {
		for (int n = 1; n <= 13; n++) {
			struct Card *crd = malloc(sizeof(struct Card));
			if (!crd)
				fatal_error("malloc() failed");

			crd->num = n;
			crd->suit = suits[s];
			crd->visible = false;
			// crd->next is initialized below
			cards[i++] = crd;
		}
	}

	shuffle(cards, 13*4);

	for (int j = 0; j < 13*4; j++)
		cards[j]->next = cards[j+1];
	return cards[0];
}

void card_free(struct Card *crd)
{
	struct Card *nxt;
	for (; crd; crd = nxt) {
		nxt = crd->next;
		free(crd);
	}
}

void card_numstr(struct Card crd, char *buf)
{
	switch(crd.num) {
	case 1:
		strcpy(buf, "A");
		break;
	case 11:
		strcpy(buf, "J");
		break;
	case 12:
		strcpy(buf, "Q");
		break;
	case 13:
		strcpy(buf, "K");
		break;
	default:
		assert(2 <= crd.num && crd.num <= 10);
		sprintf(buf, "%d", crd.num);
		break;
	}
}

void card_suitstr(struct Card crd, char *buf)
{
	switch(crd.suit) {
	case SUIT_SPADE:
		strcpy(buf, "\xe2\x99\xa0");
		break;
	case SUIT_HEART:
		strcpy(buf, "\xe2\x99\xa5");
		break;
	case SUIT_DIAMOND:
		strcpy(buf, "\xe2\x99\xa6");
		break;
	case SUIT_CLUB:
		strcpy(buf, "\xe2\x99\xa3");
		break;
	default:
		assert(0);
	}
}

void card_debug(struct Card crd)
{
	char sbuf[CARD_SUITSTRMAX], nbuf[CARD_NUMSTRMAX];
	card_suitstr(crd, sbuf);
	card_numstr(crd, nbuf);
	printf("%s%s\n", sbuf, nbuf);
}

// crd can be NULL
static struct Card *next_n_times(struct Card *crd, unsigned int n)
{
	for (unsigned int i=0; i<n && crd; i++)
		crd = crd->next;
	return crd;
}

struct Card *card_tops(struct Card *crd, unsigned int n)
{
	while (next_n_times(crd, n))
		crd = crd->next;
	return crd;
}

struct Card *card_top(struct Card *crd)
{
	return card_tops(crd, 1);
}

struct Card *card_popbot(struct Card **bot)
{
	struct Card *res = *bot;  // c funniness: res and bot are different types, but this is correct
	*bot = res->next;
	res->next = NULL;
	return res;
}

void card_pushtop(struct Card **list, struct Card *newtop)
{
	if (*list) {
		struct Card *top = card_top(*list);
		assert(!top->next);
		top->next = newtop;
	} else
		*list = newtop;
}
