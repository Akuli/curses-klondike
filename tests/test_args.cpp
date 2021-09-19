#include <vector>
#include <assert.h>
#include "../src/args.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.hpp"

static void read_file(FILE *f, const char *expected)
{
	char buf[1000];

	unsigned int n = ftell(f);
	assert(n <= sizeof(buf)-1);  // make buf bigger if this fails

	rewind(f);
	assert(fread(buf, 1, n, f) == n);
	buf[n] = 0;
	rewind(f);

	if (strcmp(buf, expected) != 0) {
		printf("\n\noutputs differ\n");
		printf("=== expected ===\n");
		printf("%s\n", expected);
		printf("=== actual ===\n");
		printf("%s\n", buf);
		abort();
	}
}

class Tester {
public:
	FILE *out, *err;
	Args args;

	Tester() {
		out = tmpfile();
		err = tmpfile();
		assert(out && err);
	}
	~Tester() {
		fclose(out);
		fclose(err);
	}
	int parse(std::vector<std::string> argvec) {
		return args_parse(args, argvec, out, err);
	}
};

TEST(args_help)
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

TEST(args_defaults)
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd" }) == -1);
	assert(t.args.color);
	assert(t.args.pick == 3);
	assert(!t.args.discardhide);
}

TEST(args_no_defaults)
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-color", "--pick=2", "--discard-hide" }) == -1);
	assert(!t.args.color);
	assert(t.args.pick == 2);
	assert(t.args.discardhide);
}

#define HELP_STUFF "\nRun 'asdasd --help' for help.\n"

TEST(args_errors)
{
	Tester t;

	// TODO: test ambiguous option if there will ever be an ambiguous option

	assert(t.parse(std::vector<std::string>{ "asdasd", "--wut" }) == 2);
	read_file(t.err, "asdasd: unknown option '--wut'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "wut" }) == 2);
	read_file(t.err, "asdasd: unexpected argument: 'wut'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-colors", "lel" }) == 2);
	read_file(t.err, "asdasd: use just '--no-colors', not '--no-colors=something' or '--no-colors something'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-colors", "--no-colors" }) == 2);
	read_file(t.err, "asdasd: repeated option '--no-colors'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--no-colors", "--no-colors", "--no-colors", "--no-colors" }) == 2);
	read_file(t.err, "asdasd: repeated option '--no-colors'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick" }) == 2);
	read_file(t.err, "asdasd: use '--pick n' or '--pick=n', not just '--pick'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick", "a" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick", "1", "2" }) == 2);
	read_file(t.err, "asdasd: unexpected argument: '2'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=a" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=0" }) == 2);
	read_file(t.err, "asdasd: '--pick' wants an integer between 1 and 24, not '0'" HELP_STUFF);

	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=1" }) == -1);
	assert(t.args.pick == 1);
}

TEST(args_nused_bug)
{
	Tester t;
	assert(t.parse(std::vector<std::string>{ "asdasd", "--pick=1", "--no-color" }) == -1);
	assert(t.args.pick == 1);
	assert(!t.args.color);  // the bug was that --no-color got ignored
}
