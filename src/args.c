/*
non-gnu-extension getopt doesn't support --long-arguments
so i did it myself, why not :D
doesn't support --, but nobody needs it for this program imo
*/

#include "args.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

enum OptType { YESNO };

struct OptSpec {
	char *name;
	enum OptType type;
	char *desc;
};

// TODO: "--pick" option for picking n cards from stock to discard at a time
static struct OptSpec option_specs[] = {
	{ "--help", YESNO, "show this help message and exit" },
	{ "--no-colors", YESNO, "don't use colors, even if the terminal supports colors" }
};

static void print_help_message(char *argv0) {
	printf("Usage: %s", argv0);
	for (unsigned int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
		switch(option_specs[i].type) {
		case YESNO:
			printf(" [%s]", option_specs[i].name);
			break;
		default:
			assert(0);
		}
	}

	puts("\n\nOptions:");
	for (unsigned int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++)
		printf("  %-12s  %s\n", option_specs[i].name, option_specs[i].desc);
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
			fprintf(stderr, "%s: ambiguous option '%s': could be '%s' or '%s'\n",
					argv0, nam, res->name, option_specs[i].name);
			return NULL;
		}
		res = &option_specs[i];
	}

	if (!res)
		fprintf(stderr, "%s: unknown option '%s'\n", argv0, nam);
	return res;
}

// returns number of args used, or -1 on error
// argc and argv should point to remaining args, not always all the args
static int tokenize(char *argv0, struct Token *tok, int argc, char **argv)
{
	assert(argc >= 1);
	if (argv[0][0] != '-' || argv[0][1] != '-' || !( 'a' <= argv[0][2] && argv[0][2] <= 'z' )) {
		fprintf(stderr, "%s: unexpected argument: '%s'\n", argv0, argv[0]);
		return -1;
	}

	int nused;
	char *nam, *val, *eq;
	bool freenam;
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
		freenam = false;
		if (argc >= 2 && argv[1][0] != '-') {
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

// returns 0 on success, negative on failure
static int check_for_duplicates(char *argv0, struct Token *toks, int ntoks)
{
	// to detect duplicates
	bool seen[sizeof(option_specs)/sizeof(option_specs[0])] = {0};

	for (int i = 0; i < ntoks; i++) {
		int j = toks[i].spec - &option_specs[0];
		if (seen[j]) {
			fprintf(stderr, "%s: repeated option '%s'\n", argv0, toks[i].spec->name);
			return -1;
		}
		seen[j] = true;
	}

	return 0;
}

static struct Args default_args = {
	.color = true
};

// returns true to keep running, or false to exit with status 0
static bool tokens_to_struct_args(char *argv0, struct Token *toks, int ntoks, struct Args *ar)
{
	memcpy(ar, &default_args, sizeof(default_args));
	for (int i = 0; i < ntoks; i++) {
#define OPTION_IS(opt) (strcmp(toks[i].spec->name, #opt) == 0)

		if OPTION_IS(--help) {
			print_help_message(argv0);
			return false;
		}

		if OPTION_IS(--no-colors)
			ar->color = false;
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

	if (check_for_duplicates(argv0, toks, ntoks) < 0)
		goto err;

	bool run = tokens_to_struct_args(argv0, toks, ntoks, ar);

	free(toks);
	return run ? -1 : 0;

err:
	free(toks);
	fprintf(stderr, "Run '%s --help' for help.\n", argv0);
	return 2;
}
