#include <assert.h>
#include <src/args.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

// these are externed in src/args.h
FILE *args_outfile;
FILE *args_errfile;


void init_args_tests(void)
{
	args_outfile = tmpfile();
	args_errfile = tmpfile();
	assert(args_outfile && args_errfile);
}

static void escape_print(char *s)
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

static void read_args_file(FILE *f, char *val)
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


// creates 2 comma-separated things
#define ARGS(...) sizeof( (char*[]){ __VA_ARGS__ } ) / sizeof(char*), (char*[]){ __VA_ARGS__ }

TEST(args_help)
{
	struct Args ar;
	assert(args_parse(&ar, ARGS("asdasd", "--help")) == 0);
	read_args_file(args_outfile,
		"Usage: asdasd [--help] [--no-colors]\n\n"
		"Options:\n"
		"  --help        show this help message and exit\n"
		"  --no-colors   don't use colors, even if the terminal supports colors\n"
	);
}

TEST(args_defaults)
{
	struct Args ar;
	assert(args_parse(&ar, ARGS("asdasd")) == -1);
	assert(ar.color);
}

TEST(args_no_defaults)
{
	struct Args ar;
	assert(args_parse(&ar, ARGS("asdasd", "--no-color")) == -1);
	assert(!ar.color);
}

#define HELP_STUFF "\nRun 'asdasd --help' for help.\n"

TEST(args_errors)
{
	struct Args ar;

	// TODO: test ambiguous option if there will ever be an ambiguous option
	// TODO: test missing value arg to option when there will be options that need a value arg

	assert(args_parse(&ar, ARGS("asdasd", "--wut")) == 2);
	read_args_file(args_errfile, "asdasd: unknown option '--wut'" HELP_STUFF);

	assert(args_parse(&ar, ARGS("asdasd", "--no-colors", "lel")) == 2);
	read_args_file(args_errfile,
		"asdasd: use just '--no-colors', not '--no-colors=something' or '--no-colors something'" HELP_STUFF);

	assert(args_parse(&ar, ARGS("asdasd", "--no-colors", "--no-colors")) == 2);
	read_args_file(args_errfile, "asdasd: repeated option '--no-colors'" HELP_STUFF);

	assert(args_parse(&ar, ARGS("asdasd", "--no-colors", "--no-colors", "--no-colors", "--no-colors")) == 2);
	read_args_file(args_errfile, "asdasd: repeated option '--no-colors'" HELP_STUFF);
}
