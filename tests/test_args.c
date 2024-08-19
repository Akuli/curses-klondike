#include "../src/args.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
    FILE *out;
    FILE *err;
    int returncode;
    struct Args args;
} test_results = {0};

static void cleanup()
{
    if (test_results.out)
        fclose(test_results.out);
    if (test_results.err)
        fclose(test_results.err);
}

static void run_parser(const char *const *argv)
{
    static bool cleanup_added = false;
    if (!cleanup_added) {
        atexit(cleanup);
        cleanup_added = true;
    }

    if (!test_results.out)
        test_results.out = tmpfile();
    if (!test_results.err)
        test_results.err = tmpfile();
    assert(test_results.out && test_results.err);

    rewind(test_results.out);
    rewind(test_results.err);

    test_results.returncode = parse_args(&test_results.args, argv, test_results.out, test_results.err);
}

static void read_file(FILE *f, const char *expected)
{
    long size = ftell(f);
    assert(size >= 0);
    rewind(f);
    assert(ftell(f) == 0);

    char *actual = malloc(size+1);
    assert(actual);
    size_t nread = fread(actual, 1, size, f);
    assert(nread == size);
    actual[size] = '\0';

    if (strcmp(actual, expected)) {
        printf("\n\noutputs differ\n");
        printf("=== expected ===\n%s\n", expected);
        printf("=== actual ===\n%s\n", actual);
        abort();
    }

    free(actual);
}

void test_args_help()
{
    run_parser((const char *[]){"asdasd", "--help", NULL});
    assert(test_results.returncode == 0);

    read_file(test_results.out,
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
    run_parser((const char *[]){"asdasd", NULL});
    assert(test_results.returncode == -1);  // keep going

    assert(test_results.args.color);
    assert(test_results.args.pick == 3);
    assert(!test_results.args.discardhide);
}

void test_args_no_defaults()
{
    run_parser((const char *[]){ "asdasd", "--no-color", "--pick=2", "--discard-hide", NULL });
    assert(test_results.returncode == -1);  // keep going

    assert(!test_results.args.color);
    assert(test_results.args.pick == 2);
    assert(test_results.args.discardhide);
}

void test_args_errors()
{
    // TODO: test ambiguous option if there will ever be an ambiguous option

    run_parser((const char *[]){ "asdasd", "--wut", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: unknown option '--wut'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "wut", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: unexpected argument: 'wut'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--no-colors", "lel", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: use just '--no-colors', not '--no-colors something' or '--no-colors=something'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--no-colors", "--no-colors", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: repeated option '--no-colors'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--no-colors", "--no-colors", "--no-colors", "--no-colors", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: repeated option '--no-colors'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: use '--pick n' or '--pick=n', not just '--pick'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick", "a", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick", "", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: '--pick' wants an integer between 1 and 24, not ''\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick", "1", "2", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: unexpected argument: '2'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick=a", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: '--pick' wants an integer between 1 and 24, not 'a'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick=0", NULL }); assert(test_results.returncode == 2);
    read_file(test_results.err, "asdasd: '--pick' wants an integer between 1 and 24, not '0'\nRun 'asdasd --help' for help.\n");

    run_parser((const char *[]){ "asdasd", "--pick=1", NULL }); assert(test_results.returncode == -1);
    assert(test_results.args.pick == 1);
}

void test_args_nused_bug()
{
    run_parser((const char *[]){ "asdasd", "--pick=1", "--no-color", NULL });
    assert(test_results.args.pick == 1);
    assert(!test_results.args.color);  // the bug was that --no-color got ignored
}
