#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

struct Args {
	bool color;
};

// returns an exit status to return from main, or -1 to keep going
int args_parse(struct Args *ar, int argc, char **argv);

#endif   // ARGS_H
