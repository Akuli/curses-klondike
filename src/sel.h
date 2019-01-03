#ifndef SEL_H
#define SEL_H

#include <stdbool.h>
#include "sol.h"
#include "ui.h"  // IWYU pragma: keep

enum SelDirection { SEL_LEFT, SEL_RIGHT, SEL_UP, SEL_DOWN };

// selects the topmost card at plc
void sel_byplace(struct Sol sol, struct UiSelection *sel, SolCardPlace plc);

// select another card at left, right, top or bottom, if possible
// NOT for moving the card
void sel_anothercard(struct Sol sol, struct UiSelection *sel, enum SelDirection dir);

// sel_anothercard(), but called when the user is moving a card around
// sel is what is being moved (card not NULL), mv is where the card is being moved
void sel_anothercardmv(struct Sol sol, struct UiSelection sel, enum SelDirection dir, SolCardPlace *mv);

// if sel is in tableau and possible to select more/less cards in that tableau item, do that
// returns true if something was done, false otherwise
bool sel_more(struct Sol sol, struct UiSelection *sel);
bool sel_less(struct Sol sol, struct UiSelection *sel);

// called when the user is done with dragging a card, moves the card if possible and resets sel
void sel_endmv(struct Sol *sol, struct UiSelection *sel, SolCardPlace mv);

// moves selected card to a foundation, if possible
void sel_2foundation(struct Sol *sol, struct UiSelection *sel);

#endif  // SEL_H
