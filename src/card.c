#include "card.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

enum Suit suit_all[4] = { SUIT_SPADE, SUIT_HEART, SUIT_DIAMOND, SUIT_CLUB };

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
	int i = 0;

	for (int s = 0; s < 4; s++) {
		for (int n = 1; n <= 13; n++) {
			struct Card *crd = malloc(sizeof(struct Card));
			if (!crd)
				fatal_error("malloc() failed");

			crd->num = n;
			crd->suit = suit_all[s];
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

void card_str(struct Card crd, char *buf)
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

	switch(crd.num) {
	case 1:
		strcat(buf, "A");
		break;
	case 11:
		strcat(buf, "J");
		break;
	case 12:
		strcat(buf, "Q");
		break;
	case 13:
		strcat(buf, "K");
		break;
	default:
		assert(2 <= crd.num && crd.num <= 10);
		sprintf(buf + strlen(buf), "%d", crd.num);
		break;
	}
}

void card_debug(struct Card crd)
{
	char buf[CARD_STRMAX];
	card_str(crd, buf);
	printf("%s\n", buf);
}

struct Card *card_top(struct Card *crd)
{
	while (crd->next)
		crd = crd->next;
	return crd;
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
