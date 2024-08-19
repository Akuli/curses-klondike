/*
Non-gnu-extension getopt doesn't support --long-arguments, so i did it myself, why not :D

Not very general, doesn't support various features that this program doesn't need.

Note about error handling: Most functions print an error message and return
something to indicate failure (e.g. false, -1). Then a message like
"Run './cursesklon --help' for help" is printed in one place, so that every
error message gets it added when the error indicator is returned.
*/

#define _POSIX_C_SOURCE 200809L   // for strdup()

#include "args.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct ArgParser {
    FILE *out;  // stdout or temporary file during tests
    FILE *err;  // stdout or temporary file during tests
    const char *argv0;  // first command line argument (program name)
};

enum OptionType { OPT_YESNO, OPT_INT };

struct OptionSpec {
    const char *name;
    const char *metavar;  // may be NULL
    enum OptionType type;
    int min, max;   // Meaningful only for integer options
    const char *desc;
};

static const struct OptionSpec option_specs[] = {
    { .name = "--help", .type = OPT_YESNO, .desc = "show this help message and exit" },
    { .name = "--no-colors", .type = OPT_YESNO, .desc = "don't use colors, even if the terminal supports colors" },
    { .name = "--pick", .type = OPT_INT, .metavar = "n", .min = 1, .max = 13*4 - (1+2+3+4+5+6+7), .desc = "pick n cards from stock at a time, default is 3" },
    { .name = "--discard-hide", .type = OPT_YESNO, .desc = "only show topmost discarded card (not useful with --pick=1)" }
};

/*
This prints the "--help" message that lists and describes all options.
*/
static void print_help(const struct ArgParser *parser)
{
    fprintf(parser->out, "Usage: %s", parser->argv0);
    for (int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
        const struct OptionSpec *spec = &option_specs[i];
        if (spec->metavar)
            fprintf(parser->out, " [%s %s]", spec->name, spec->metavar);
        else
            fprintf(parser->out, " [%s]", spec->name);
    }

    fprintf(parser->out, "\n\nOptions:\n");
    for (int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
        const struct OptionSpec *spec = &option_specs[i];

        char name_with_metavar[500];
        if (spec->metavar)
            snprintf(name_with_metavar, sizeof(name_with_metavar), "%s %s", spec->name, spec->metavar);
        else
            snprintf(name_with_metavar, sizeof(name_with_metavar), "%s", spec->name);

        fprintf(parser->out, "  %-14s  %s\n", name_with_metavar, spec->desc);
    }

}

/*
Determine whether a string starts with a prefix.
*/
static bool starts_with(const char *str, const char *prefix)
{
    return !strncmp(str, prefix, strlen(prefix));
}

/*
Returns the option that starts with prefix, if only one such match exists.

For example, --p is a valid way to abbreviate --pick. This works in most
argument parsers.
*/
const struct OptionSpec *find_from_option_specs(const struct ArgParser *parser, const char *prefix)
{
    const struct OptionSpec *match = NULL;

    for (int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
        const struct OptionSpec *spec = &option_specs[i];
        if (starts_with(spec->name, prefix)) {
            if (match == NULL) {
                match = spec;
            } else {
                fprintf(
                    parser->err,
                    "%s: ambiguous option '%s': could be '%s' or '%s'\n",
                    parser->argv0, prefix, match->name, spec->name);
                return NULL;
            }
        }
    }

    if (!match) {
        fprintf(parser->err, "%s: unknown option '%s'\n", parser->argv0, prefix);
        return NULL;
    }

    return match;
}

/*
Represents an option and its value, if any.

This comes from one or two command-line (argv) arguments. For example:

    --pick 3    --> Token{.spec = ..., .value = "3"}
    --pick=3    --> Token{.spec = ..., .value = "3"}
    --no-colors --> Token{.spec = ..., .value = NULL}
*/
struct Token {
    const struct OptionSpec *spec;  // pointer into option_specs array
    const char *value;  // may be NULL
};

/*
Tests if a string represents an integer that is between the minimum and maximum bounds.
*/
static bool is_valid_integer(const char *str, int min, int max)
{
    for (int i = 0; str[i] != '\0'; i++)
        if (!('0' <= str[i] && str[i] <= '9'))
            return false;

    int n = atoi(str);
    return min <= n && n <= max;
}

/*
Creates one from command-line args.

argv does not include argv0 and is incremented to skip processed arguments.

Return value false indicates error.
*/
static bool get_token(const struct ArgParser *parser, struct Token *dest, const char *const **argv)
{
    const char *first_arg = **argv;
    assert(first_arg != NULL);
    ++*argv;

    if (strlen(first_arg) < 3 || !starts_with(first_arg, "--")) {
        fprintf(parser->err, "%s: unexpected argument: '%s'\n", parser->argv0, first_arg);
        return false;
    }

    char *name = strdup(first_arg);
    assert(name);
    const char *value;

    const char *eq = strstr(first_arg, "=");
    if (eq) {
        value = eq+1;
        *strstr(name, "=") = '\0';  // truncate the name
    } else {
        const char *next_arg = **argv;
        if (next_arg == NULL || starts_with(next_arg, "-")) {
            // No more arguments, or next argument is another option
            // e.g. "--discardhide --pick=3" sees the initial '-' of "--pick=3"
            value = NULL;
        } else {
            value = next_arg;
            ++*argv;
        }
    }

    const struct OptionSpec *spec = find_from_option_specs(parser, name);
    free(name);

    if (!spec)
        return false;

    switch(spec->type) {
    case OPT_YESNO:
        assert(!spec->metavar);

        if (value) {
            fprintf(
                parser->err,
                "%s: use just '%s', not '%s something' or '%s=something'\n",
                parser->argv0, spec->name, spec->name, spec->name);
            return false;
        }
        break;

    case OPT_INT:
        assert(spec->metavar);

        if (!value) {
            fprintf(
                parser->err,
                "%s: use '%s %s' or '%s=%s', not just '%s'\n",
                parser->argv0, spec->name, spec->metavar, spec->name, spec->metavar, spec->name);
            return false;
        }

        if (!is_valid_integer(value, spec->min, spec->max)) {
            fprintf(
                parser->err,
                "%s: '%s' wants an integer between %d and %d, not '%s'\n",
                parser->argv0, spec->name, spec->min, spec->max, value);
            return false;
        }

        break;
    }

    dest->spec = spec;
    dest->value = value;
    return true;
}

/*
Creates tokens from command-line args.

argv does not include argv0. Return value -1 indicates error.
*/
static int tokenize(const struct ArgParser *parser, struct Token *tokens, size_t max_tokens, const char *const *argv)
{
    int ntokens = 0;
    while (*argv != NULL) {
        struct Token t;
        if (!get_token(parser, &t, &argv))
            return -1;

        if (ntokens == max_tokens) {
            // TODO: test this
            fprintf(parser->err, "%s: too many command-line arguments\n", parser->argv0);
            return -1;
        }

        tokens[ntokens++] = t;
    }

    return ntokens;
}

/*
This function populates the args struct or decides to print help message instead.
*/
static void tokens_to_args(const struct Token *tokens, int ntokens, struct Args *args, bool *need_help)
{
    *need_help = false;
    *args = (struct Args){
        // default values of arguments
        .color = true,
        .pick = 3,
        .discardhide = false,
    };

    for (const struct Token *t = tokens; t < &tokens[ntokens]; t++) {
        if (!strcmp(t->spec->name, "--help"))
            *need_help = true;
        else if (!strcmp(t->spec->name, "--no-colors"))
            args->color = false;
        else if (!strcmp(t->spec->name, "--pick"))
            args->pick = atoi(t->value);
        else if (!strcmp(t->spec->name, "--discard-hide"))
            args->discardhide = true;
        else
            assert(0);
    }
}

/*
Print an error and return false if the same option is passed twice.

If the same option has different values (e.g. --pick 2 --pick 3), that is still
considered as a duplicate.
*/
static bool check_duplicate_tokens(const struct ArgParser *parser, const struct Token *tokens, int ntokens)
{
    for (const struct Token *t1 = tokens; t1 < &tokens[ntokens]; t1++) {
        for (const struct Token *t2 = tokens; t2 < t1; t2++) {
            if (t1->spec == t2->spec) {
                fprintf(parser->err, "%s: repeated option '%s'\n", parser->argv0, t1->spec->name);
                return false;
            }
        }
    }

    return true;
}

/*
The "main function" of this file.
*/
int parse_args(struct Args *args_out, const char *const *argv_in, FILE *out, FILE *err)
{
    struct ArgParser parser = { .out = out, .err = err, .argv0 = *argv_in++ };
    assert(parser.argv0 != NULL);

    struct Token tokens[100];
    int ntokens = tokenize(&parser, tokens, sizeof(tokens)/sizeof(tokens[0]), argv_in);

    if (ntokens == -1 || !check_duplicate_tokens(&parser, tokens, ntokens)) {
        fprintf(parser.err, "Run '%s --help' for help.\n", parser.argv0);
        return 2;  // exit with failure
    }

    bool need_help;
    tokens_to_args(tokens, ntokens, args_out, &need_help);
    if (need_help) {
        print_help(&parser);
        return 0;  // exit with success
    }

    return -1;  // keep going with program
}
