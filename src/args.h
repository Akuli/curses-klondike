#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdio.h>

struct Args {
    bool color;
    int pick;
    bool discardhide;
};

/*
Parses command-line arguments, e.g. "--pick 10".

Usage:

    int status = parse_args(&args, (const char *[]){"./cursesklon", "--pick", "3", NULL}, stdout, stderr);

The argc passed to main() is not used. It is unnecessary because C standard
specifies that argv ends with NULL.

Status -1 indicates that arguments were parsed successfully and program should
proceed. Any other status is an exit code to be returned from main().
*/
int parse_args(struct Args *args_out, const char *const *argv_in, FILE *out, FILE *err);

#endif   // ARGS_H
