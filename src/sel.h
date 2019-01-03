#ifndef SEL_H
#define SEL_H

#include "sol.h"
#include "ui.h"  // IWYU pragma: keep

enum SelDirection { SEL_LEFT, SEL_RIGHT, SEL_UP, SEL_DOWN };

// select another card at left, right, top or bottom, if possible
// NOT for moving the card
void sel_anothercard(struct Sol sol, struct UiSelection *sel, enum SelDirection dir);

// sel_anothercard(), but called when the user is moving a card around
// sel is what is being moved (card not NULL), mv is where the card is being moved
void sel_anothercardmv(struct Sol sol, struct UiSelection sel, enum SelDirection dir, SolCardPlace *mv);

// called when the user is done with dragging a card, moves the card if possible and resets sel
void sel_endmv(struct Sol *sol, struct UiSelection *sel, SolCardPlace mv);

#endif  // SEL_H
