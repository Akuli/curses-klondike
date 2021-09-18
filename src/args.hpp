#ifndef ARGS_H
#define ARGS_H

#include <vector>
#include <string>
#include <stdio.h>

struct Args {
	bool color = true;
	unsigned int pick = 3;
	bool discardhide = false;
};

// main.c sets these to stdout and stderr, tests/test_args.c sets these to temporary files
extern FILE *args_outfile;
extern FILE *args_errfile;

// returns an exit status to return from main, or -1 to keep going
int args_parse(Args& ar, std::vector<std::string> argvec);

#endif   // ARGS_H
