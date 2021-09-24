#ifndef ARGS_H
#define ARGS_H

#include <iostream>
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
std::optional<Args> args_parse(int& status, const std::vector<std::string>& args, std::ostream& out, std::ostream& err);

#endif   // ARGS_H
