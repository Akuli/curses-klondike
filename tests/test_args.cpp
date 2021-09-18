#include <vector>
#include <assert.h>
#include "../src/args.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.hpp"

FILE *args_test_out;
FILE *args_test_err;

void init_args_tests(void)
{
	args_test_out = tmpfile();
	args_test_err = tmpfile();
	assert(args_test_out && args_test_err);
}

void deinit_args_tests(void)
{
	fclose(args_test_out);
	fclose(args_test_err);
}

static void escape_print(const char *s)
{
	putchar('"');
	char ch;
	while( (ch = *s++) ) {
		switch(ch) {
		case '\n':
			putchar('\\');
			putchar('n');
			break;
		default:
			putchar(ch);
			break;
		}
	}
	putchar('"');
}

static void read_args_file(FILE *f, const char *val)
{
	char buf[1000];

	unsigned int n = ftell(f);
	assert(n <= sizeof(buf)-1);  // make buf bigger if this fails

	rewind(f);
	assert(fread(buf, 1, n, f) == n);
	buf[n] = 0;
	rewind(f);

	if (strcmp(buf, val) != 0) {
		printf("\n\noutputs differ\n");
		printf("%-8s: ", "expected");
		escape_print(val);
		printf("\n");
		printf("%-8s: ", "got");
		escape_print(buf);
		printf("\n");
		abort();
	}
}


static int parse(Args& ar, std::vector<std::string> argvec)
{
	return args_parse(ar, argvec, args_test_out, args_test_err);
}

TEST(args_help)
{
	Args ar;
	assert(parse(ar, std::vector<std::string>{ "asdasd", "--help" }) == 0);
	read_args_file(args_test_out,
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
	Args ar;
	assert(parse(ar, std::vector<std::string>{ "asdasd" }) == -1);
	assert(ar.color);
	assert(ar.pick == 3);
	assert(!ar.discardhide);
}

TEST(args_no_defaults)
{
	Args ar;
	assert(parse(ar, std::vector<std::string>{ "asdasd", "--no-color", "--pick=2", "--discard-hide" }) == -1);
	assert(!ar.color);
	assert(ar.pick == 2);
	assert(ar.discardhide);
}

#define HELP_STUFF "\nRun 'asdasd --help' for help.\n"

TEST(args_errors)
{
	Args ar;

	// TODO: test ambiguous option if there will ever be an ambiguous option

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--wut" }) == 2);
	read_args_file(args_test_err, "asdasd: unknown option '--wut'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "wut" }) == 2);
	read_args_file(args_test_err, "asdasd: unexpected argument: 'wut'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--no-colors", "lel" }) == 2);
	read_args_file(args_test_err,
		"asdasd: use just '--no-colors', not '--no-colors=something' or '--no-colors something'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--no-colors", "--no-colors" }) == 2);
	read_args_file(args_test_err, "asdasd: repeated option '--no-colors'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--no-colors", "--no-colors", "--no-colors", "--no-colors" }) == 2);
	read_args_file(args_test_err, "asdasd: repeated option '--no-colors'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick" }) == 2);
	read_args_file(args_test_err, "asdasd: use '--pick n' or '--pick=n', not just '--pick'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick", "a" }) == 2);
	read_args_file(args_test_err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick", "1", "2" }) == 2);
	read_args_file(args_test_err, "asdasd: unexpected argument: '2'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick=a" }) == 2);
	read_args_file(args_test_err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick=0" }) == 2);
	read_args_file(args_test_err, "asdasd: '--pick' wants an integer between 1 and 24, not '0'" HELP_STUFF);

	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick=1" }) == -1);
	assert(ar.pick == 1);
}

TEST(args_nused_bug)
{
	Args ar;
	assert(parse(ar, std::vector<std::string>{ "asdasd", "--pick=1", "--no-color" }) == -1);
	assert(ar.pick == 1);
	assert(!ar.color);  // the bug was that --no-color got ignored
}
