#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdio.h>

struct Args {
	bool color;
	unsigned int pick;
	bool discardhide;
};

// main.c sets these to stdout and stderr, tests/test_args.c sets these to temporary files
extern FILE *args_outfile;
extern FILE *args_errfile;

// returns an exit status to return from main, or -1 to keep going
int args_parse(struct Args *ar, int argc, char *const *argv);

#endif   // ARGS_H
