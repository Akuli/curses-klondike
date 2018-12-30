#include "card.h"
#include <stdlib.h>
#include "misc.h"

enum Suit suit_all[4] = { SPADE, HEART, DIAMOND, CLUB };

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

	for (unsigned int s = 0; s < 4; s++) {
		for (unsigned int n = 1; n <= 13; n++) {
			struct Card *crd = malloc(sizeof(struct Card));
			if (!crd)
				fatal_error("malloc() failed");

			crd->num = n;
			crd->suit = suit_all[s];
			crd->visible = false;
			// crd->next is left uninitialized for valgrinding
			cards[i++] = crd;
		}
	}

	shuffle(cards, 13*4);

	for (unsigned int j = 0; j < 13*4; j++)
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
