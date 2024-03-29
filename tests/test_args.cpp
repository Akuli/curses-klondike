#include "../src/args.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static void read_file(std::ostringstream& f, const std::string& expected)
{
	if (f.str() != expected) {
		std::cout << "\n\noutputs differ\n";
		std::cout << "=== expected ===\n" << expected << std::endl;
		std::cout << "=== actual ===\n"   << f.str()  << std::endl;
		abort();
	}
	f = std::ostringstream();
}

class Tester {
public:
	std::ostringstream out;
	std::ostringstream err;
	Args args;

	~Tester() {
		read_file(this->out, "");
		read_file(this->err, "");
	}
	int parse(std::vector<std::string> arg_vector) {
		int status = -1;
		std::optional<Args> args = parse_args(status, arg_vector, this->out, this->err);
		if (args)
			this->args = args.value();
		return status;
	}
};

void test_args_help()
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd", "--help" }) == 0);
	read_file(t.out,
		"Usage: asdasd [--help] [--no-colors] [--pick n] [--discard-hide]\n\n"
		"Options:\n"
		"  --help          show this help message and exit\n"
		"  --no-colors     don't use colors, even if the terminal supports colors\n"
		"  --pick n        pick n cards from stock at a time, default is 3\n"
		"  --discard-hide  only show topmost discarded card (not useful with --pick=1)\n"
	);
}

void test_args_defaults()
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd" }) == -1);
	assert(t.args.color);
	assert(t.args.pick == 3);
	assert(!t.args.discardhide);
}

void test_args_no_defaults()
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-color", "--pick=2", "--discard-hide" }) == -1);
	assert(!t.args.color);
	assert(t.args.pick == 2);
	assert(t.args.discardhide);
}

void test_args_errors()
{
	Tester t;
	std::string help = "\nRun 'asdasd --help' for help.\n";

	// TODO: test ambiguous option if there will ever be an ambiguous option

	assert(t.parse(std::vector<std::string>{ "asdasd", "--wut" }) == 2);
	read_file(t.err, "asdasd: unknown option '--wut'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "wut" }) == 2);
	read_file(t.err, "asdasd: unexpected argument: 'wut'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-colors", "lel" }) == 2);
	read_file(t.err, "asdasd: use just '--no-colors', not '--no-colors something' or '--no-colors=something'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-colors", "--no-colors" }) == 2);
	read_file(t.err, "asdasd: repeated option '--no-colors'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-colors", "--no-colors", "--no-colors", "--no-colors" }) == 2);
	read_file(t.err, "asdasd: repeated option '--no-colors'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick" }) == 2);
	read_file(t.err, "asdasd: use '--pick n' or '--pick=n', not just '--pick'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick", "a" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick", "" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not ''" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick", "1", "2" }) == 2);
	read_file(t.err, "asdasd: unexpected argument: '2'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=a" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=0" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not '0'" + help);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=1" }) == -1);
	assert(t.args.pick == 1);
}

void test_args_nused_bug()
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=1", "--no-color" }) == -1);
	assert(t.args.pick == 1);
	assert(!t.args.color);  // the bug was that --no-color got ignored
}
