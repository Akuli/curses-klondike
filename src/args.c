/*
non-gnu-extension getopt doesn't support --long-arguments
so i did it myself, why not :D
doesn't support --, but nobody needs it for this program imo
*/

#include "args.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

enum OptType { TYPE_YESNO, TYPE_INT };

struct OptSpec {
	char *name;
	char *metavar;
	enum OptType type;
	int min;
	int max;
	char *desc;
};

// TODO: document env vars in --help
static struct OptSpec option_specs[] = {
	{ "--help", NULL, TYPE_YESNO, 0, 0, "show this help message and exit" },
	{ "--no-colors", NULL, TYPE_YESNO, 0, 0, "don't use colors, even if the terminal supports colors" },
	{ "--pick", "n", TYPE_INT, 1, 13*4 - (1+2+3+4+5+6+7), "pick n cards from stock at a time, default is 3" }
};

static void print_help_option(struct OptSpec opt)
{
	char *pre;
	bool freepre = false;

	switch(opt.type) {
	case TYPE_YESNO:
		pre = opt.name;
		break;
	default:
		if (!( pre = malloc(strlen(opt.name) + 1 + strlen(opt.metavar) + 1) ))
			fatal_error("malloc() failed");
		freepre = true;

		strcpy(pre, opt.name);
		strcat(pre, " ");
		strcat(pre, opt.metavar);
		break;
	}

	fprintf(args_outfile, "  %-12s  %s\n", pre, opt.desc);
	if (freepre)
		free(pre);
}

static void print_help(char *argv0)
{
	fprintf(args_outfile, "Usage: %s", argv0);
	for (unsigned int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
		switch(option_specs[i].type) {
		case TYPE_YESNO:
			fprintf(args_outfile, " [%s]", option_specs[i].name);
			break;
		case TYPE_INT:
			fprintf(args_outfile, " [%s %s]", option_specs[i].name, option_specs[i].metavar);
			break;
		default:
			assert(0);
		}
	}

	fprintf(args_outfile, "\n\nOptions:\n");
	for (unsigned int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++)
		print_help_option(option_specs[i]);
}

static bool startswith(char *s, char *pre)
{
	return (strstr(s, pre) == s);
}

// represents an option and a value, if any
// needed because one arg token can come from 2 argv items: --pick 3
// or just 1: --no-colors, --pick=3
struct Token {
	struct OptSpec *spec;  // pointer to an item of option_specs
	char *value;  // from argv, don't free()
};

static struct OptSpec *find_from_option_specs(char *argv0, char *nam)
{
	struct OptSpec *res = NULL;
	for (unsigned int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
		if (!startswith(option_specs[i].name, nam))
			continue;

		if (res) {
			fprintf(args_errfile, "%s: ambiguous option '%s': could be '%s' or '%s'\n",
					argv0, nam, res->name, option_specs[i].name);
			return NULL;
		}
		res = &option_specs[i];
	}

	if (!res)
		fprintf(args_errfile, "%s: unknown option '%s'\n", argv0, nam);
	return res;
}

// returns number of args used, or -1 on error
// argc and argv should point to remaining args, not always all the args
static int tokenize(char *argv0, struct Token *tok, int argc, char **argv)
{
	assert(argc >= 1);
	if (argv[0][0] != '-' || argv[0][1] != '-' || !( 'a' <= argv[0][2] && argv[0][2] <= 'z' )) {
		fprintf(args_errfile, "%s: unexpected argument: '%s'\n", argv0, argv[0]);
		return -1;
	}

	int nused;
	char *nam, *val, *eq;
	bool freenam = false;
	if ( (eq = strchr(argv[0], '=')) ) {
		nused = 2;
		freenam = true;
		nam = malloc(eq - argv[0] + 1);
		if (!nam)
			fatal_error("malloc() failed");

		memcpy(nam, argv[0], eq - argv[0]);
		nam[ eq - argv[0] ] = 0;
		val = eq+1;
	} else {
		nam = argv[0];
		if (argc >= 2 && argv[1][0] != '-') {  // TODO: handle negative numbers as arguments?
			nused = 2;
			val = argv[1];
		} else {
			nused = 1;
			val = NULL;
		}
	}

	struct OptSpec *spec = find_from_option_specs(argv0, nam);
	if (!spec)
		goto err;

	tok->spec = spec;
	tok->value = val;
	goto out;

err:
	nused = -1;
	// "fall through" to out
out:
	if (freenam)
		free(nam);
	return nused;
}

// returns true on success, false on failure
static int is_valid_integer(char *str, int min, int max)
{
	// see the example in linux man-pages strtol(3)
	errno = 0;
	char *endptr;
	long val = strtol(str, &endptr, 10);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
			|| (errno != 0 && val == 0)) {
		errno = 0;
		return false;
	}

	if (endptr == str)   // no digits found
		return false;

	if (*endptr)   // further characters after number
		return false;

	return( (long)min <= val && val <= (long)max );
}

// returns 0 on success, negative on failure
static int check_token_by_type(char *argv0, struct Token tok)
{
	switch(tok.spec->type) {
	case TYPE_YESNO:
		if (tok.value) {
			fprintf(args_errfile, "%s: use just '%s', not '%s=something' or '%s something'\n",
					argv0, tok.spec->name, tok.spec->name, tok.spec->name);
			return -1;
		}
		break;

	case TYPE_INT:
		if (!tok.value) {
			fprintf(args_errfile, "%s: use '%s %s' or '%s=%s', not just '%s'\n",
					argv0, tok.spec->name, tok.spec->metavar, tok.spec->name, tok.spec->metavar, tok.spec->name);
			return -1;
		}
		if (!is_valid_integer(tok.value, tok.spec->min, tok.spec->max)) {
			fprintf(args_errfile, "%s: '%s' wants an integer between %d and %d, not '%s'\n",
					argv0, tok.spec->name, tok.spec->min, tok.spec->max, tok.value);
			return -1;
		}
		break;

	default:
		assert(0);
	}

	return 0;
}

// returns 0 on success, negative on failure
static int check_tokens(char *argv0, struct Token *toks, int ntoks)
{
	// to detect duplicates
	bool seen[sizeof(option_specs)/sizeof(option_specs[0])] = {0};

	for (int i = 0; i < ntoks; i++) {
		int j = toks[i].spec - &option_specs[0];
		if (seen[j]) {
			fprintf(args_errfile, "%s: repeated option '%s'\n", argv0, toks[i].spec->name);
			return -1;
		}
		seen[j] = true;

		if (check_token_by_type(argv0, toks[i]) < 0)
			return -1;
	}

	return 0;
}

static struct Args default_args = {
	.color = true,
	.pick = 3
};

// returns true to keep running, or false to exit with status 0
static bool tokens_to_struct_args(char *argv0, struct Token *toks, int ntoks, struct Args *ar)
{
	memcpy(ar, &default_args, sizeof(default_args));
	for (int i = 0; i < ntoks; i++) {
#define OPTION_IS(opt) (strcmp(toks[i].spec->name, #opt) == 0)

		if OPTION_IS(--help) {
			print_help(argv0);
			return false;
		}

		if OPTION_IS(--no-colors)
			ar->color = false;
		else if OPTION_IS(--pick)
			ar->pick = atoi(toks[i].value);  // atoi won't fail, the value is already validated
		else
			assert(0);

#undef OPTION_IS
	}

	return true;
}

int args_parse(struct Args *ar, int argc, char **argv)
{
	char *argv0 = *argv++;
	argc--;

	// a token comes from 1 or 2 args, so there are at most argc tokens
	struct Token *toks = malloc(sizeof(struct Token) * argc);
	if (!toks)
		fatal_error("malloc() failed");
	int ntoks = 0;

	while (argc > 0) {
		int n = tokenize(argv0, &toks[ntoks++], argc, argv);
		if (n < 0)
			goto err;
		argc -= n;
		argv += n;
	}

	if (check_tokens(argv0, toks, ntoks) < 0)
		goto err;

	bool run = tokens_to_struct_args(argv0, toks, ntoks, ar);

	free(toks);
	return run ? -1 : 0;

err:
	free(toks);
	fprintf(args_errfile, "Run '%s --help' for help.\n", argv0);
	return 2;
}
