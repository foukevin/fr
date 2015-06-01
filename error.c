#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* exit */
#include <string.h> /* strdup */

static char *name = NULL;

void setprogname(const char *progname)
{
	const char *p;

	/* free name if we already have one */
	free(name);

	for (p = progname; *progname != '\0'; progname++)
		if (*progname == '/') /* TODO: handle '\\' separators */
			p = progname + 1;

	name = strdup(p);
	/* TODO: free name on exit */
}

static void report(const char *prefix, const char *fmt, va_list params)
{
	fprintf(stderr, "%s", (name != NULL) ? name : "(unknown)");
	if (prefix != NULL)
		fprintf(stderr, ": %s", prefix);
	if (fmt != NULL) {
		fprintf(stderr, ": ");
		vfprintf(stderr, fmt, params);
	}
	fprintf(stderr, "\n");
}

void die(const char *err, ...)
{
	va_list params;
	va_start(params, err);
	report("fatal", err, params);
	va_end(params);
	exit(1);
}

#undef error
void error(const char *err, ...)
{
	va_list params;
	va_start(params, err);
	report("error", err, params);
	va_end(params);
	exit(1);
}

void warning(const char *warn, ...)
{
	va_list params;
	va_start(params, warn);
	report("warning: ", warn, params);
	va_end(params);
}
