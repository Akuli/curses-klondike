#ifndef ARGS_H
#define ARGS_H

#include <cstdio>  // c++ io streams suck ass https://stackoverflow.com/a/15106194
#include <optional>
#include <string>
#include <vector>

struct Args {
	bool color = true;
	int pick = 3;
	bool discardhide = false;
};

// program should exit with status when this returns nullopt
// using FILE* because printf
std::optional<Args> args_parse(int& status, const std::vector<std::string>& args, FILE *out, FILE *err);

#endif   // ARGS_H
