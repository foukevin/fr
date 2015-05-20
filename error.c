#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* exit */

static void report(const char *prefix, const char *fmt, va_list params)
{
	char msg[4096];
	vsnprintf(msg, sizeof(msg), fmt, params);
	fprintf(stderr, "%s%s\n", prefix, msg);
}

void die(const char *err, ...)
{
	va_list params;
	va_start(params, err);
	report("fatal: ", err, params);
	va_end(params);
	exit(1);
}

#undef error
void error(const char *err, ...)
{
	va_list params;
	va_start(params, err);
	report("error: ", err, params);
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
