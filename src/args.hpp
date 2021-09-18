#ifndef ARGS_H
#define ARGS_H

#include <vector>
#include <string>
#include <cstdio>  // c++ io streams suck ass https://stackoverflow.com/a/15106194

struct Args {
	bool color = true;
	unsigned int pick = 3;
	bool discardhide = false;
};

// returns an exit status to return from main, or -1 to keep going
int args_parse(Args& ar, std::vector<std::string> argvec, FILE *out, FILE *err);

#endif   // ARGS_H
