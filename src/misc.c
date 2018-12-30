#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

void fatal_error(char *msg)
{
	if (onerrorexit)
		onerrorexit();

	if (errno)
		fprintf(stderr, "%s: %s (errno = %d)\n", msg, strerror(errno), errno);
	else
		fprintf(stderr, "%s\n", msg);
	fflush(stderr);
	abort();
}
