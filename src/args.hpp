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
std::optional<Args> parse_args(int& status, const std::vector<std::string>& args, std::ostream& out, std::ostream& err);

#endif   // ARGS_H
