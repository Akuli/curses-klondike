#ifndef SEL_H
#define SEL_H

#include <stdbool.h>
#include "klon.h"
#include "ui.h"  // IWYU pragma: keep

enum SelDirection { SEL_LEFT, SEL_RIGHT, SEL_UP, SEL_DOWN };

// selects the topmost card at plc
void sel_byplace(struct Klon kln, struct UiSelection *sel, KlonCardPlace plc);

// select another card at left, right, top or bottom, if possible
// NOT for moving the card
void sel_anothercard(struct Klon kln, struct UiSelection *sel, enum SelDirection dir);

// sel_anothercard(), but called when the user is moving a card around
// sel is what is being moved (card not NULL), mv is where the card is being moved
void sel_anothercardmv(struct Klon kln, struct UiSelection sel, enum SelDirection dir, KlonCardPlace *mv);

// if sel is in tableau and possible to select more/less cards in that tableau item, do that
// returns true if something was done, false otherwise
bool sel_more(struct Klon kln, struct UiSelection *sel);
bool sel_less(struct Klon kln, struct UiSelection *sel);

// called when the user is done with dragging a card, moves the card if possible and resets sel
void sel_endmv(struct Klon *kln, struct UiSelection *sel, KlonCardPlace mv);

#endif  // SEL_H
