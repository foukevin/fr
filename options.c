#include "fr.h"
#include "error.h"

#include <getopt.h>
#include <stddef.h> /* NULL */
#include <stdlib.h> /* exit */
#include <string.h>

static const char *version = "0.1";

static char *mystrdup(const char *s)
{
	char *d;
	int len = strlen(s);
	d = malloc(len + 1);
	if (!d)
		die("out of memory");

	strncpy(d, s, len);
	d[len] = '\0';
	return d;
}

static void usage(const struct fr *fr)
{
	printf("Font rasterizer version %s\n", version);
	printf("Usage: %s [options] font\n", fr->progname);
	printf("Options:\n");
	printf("  --help                   Display this information\n");
	printf("  -o=<file>                Place the output atlas png into <file>\n");
	printf("  -m=<file>                Place the output font metrics into <file>\n");
	printf("  -W=<n>                   Set atlas width to <n>\n");
	printf("  -H=<n>                   Set atlas height to <n>\n");
	printf("  -s=<n>                   Render glyphs with height of <n> pixels\n");
	printf("  -p=<n>                   Pad glyph with <n> pixels\n");
	printf("  -b=<n>                   Glyph border of <n> pixels\n");
	printf("  --no-antialias           Render glyphs without antialiasing\n");
	printf("  --metrics-format=[text|binary]\n"
	       "                           Write metrics as text or binary\n");
	printf("  --rune=,<range>          Comma separated unicode point or point ranges\n");
	printf("Notes:\n");
	printf("  Ranges are in the form <c>, <l>:<u> or <l>+<n>; "
	       "of single code point <c>, lower bound <l>, upper bound <u> and extend <n>\n");
	exit(0);
}

static struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "no-antialias", no_argument, 0, 'a' },
	{ "metrics-format", required_argument, 0, 'f' },
	{ "rune", required_argument, 0, 'r' },
	{ 0, 0, 0, 0 }
};

/*
 * Extracts the range from the string into lo/hi.
 * Returns 0 (no error) or 1 whether the range is valid or not.
 * For now there are 3 types of range:
 * - simple digit
 * - explicit range in the form <lower bound>:<upper bound>
 * - in the form <lower bound>+<count>
 */
static int extract_range(int *lo, int *hi, const char *s)
{
	char *endptr;

	if (s == NULL)
		return 1;

	/* strtol sets endptr to the first non digit character. */
	*lo = strtol(s, &endptr, 0);
	if (endptr == s) /* no digit found */
		return 1;
	s = endptr;

	switch (*s++) {
	case ':':
		*hi = strtol(s, &endptr, 0);
		break;
	case '+':
		*hi = *lo + strtol(s, &endptr, 0);
		break;
	case '\0':
		*hi = *lo;
		break;
	default:
		/* invalid range */
		return 1;
	}

	return (*lo > *hi) || (*lo < 0);
}

/*
 * Returns the requested metrics format.
 * Returns -1 if the format is not a valid one.
 */
static int get_metrics_format(const char *s)
{
	if (!strcmp(s, "text"))
		return MF_TEXT;
	else if (!strcmp(s, "binary"))
		return MF_BINARY;
	return -1;
}

static int get_ranges(const char *s, struct fr *fr)
{
	int lo, hi, err = 0;
	const char *delim = ",";

	/* strtok modifies the string so duplicate it first */
	char *copy = mystrdup(s);
	const char *tok = strtok(copy, delim);

	while (tok) {
		if (!extract_range(&lo, &hi, tok)) {
			range_t *range = malloc(sizeof(range_t));
			range->lo = lo;
			range->hi = hi;
			range->next = fr->ranges;
			fr->ranges = range;
		} else {
			warning("invalid range %s\n", tok);
			err = 1;
			break;
		}
		tok = strtok(NULL, delim);
	}

	free(copy);
	return err;
}

int fr_getopt(struct fr *fr)
{
	return getopt_long(fr->argc, fr->argv, "hvao:m:W:H:s:p:b:f:", long_options, NULL);
}

void parse_options(struct fr *fr)
{
	int opt;
	int invalid_arg = 0;

	while ((opt = fr_getopt(fr)) != -1) {
		switch (opt) {
		case 'h':
			usage(fr);
			break;
		case 'v':
			fr->option_verbose = 1;
			break;
		case 'a':
			fr->no_antialias = 1;
			break;
		case 'o':
			fr->atlas_filename = mystrdup(optarg);
			break;
		case 'm':
			fr->metrics_filename = mystrdup(optarg);
			break;
		case 'W':
			fr->atlas_width = atoi(optarg);
			if (fr->atlas_width <= 0) {
				error("invalid atlas width: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'H':
			fr->atlas_height = atoi(optarg);
			if (fr->atlas_height <= 0) {
				error("invalid atlas height: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 's':
			fr->pixel_height = atoi(optarg);
			if (fr->pixel_height <= 0) {
				error("invalid size: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'p':
			fr->padding = atoi(optarg);
			if (fr->padding < 0) {
				error("invalid padding: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'b':
			fr->border = atoi(optarg);
			if (fr->border < 0) {
				error("invalid border: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'r':
			get_ranges(optarg, fr);
			break;
		case 'f':
			fr->format = get_metrics_format(optarg);
			if (fr->format == -1) {
				error("invalid metrics format: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case '?':
		default:
			error("invalid option: '%c'", optopt);
			break;
		}

		if (invalid_arg)
			exit(1);
	}

	/* Handle non-option arguments (ie: font names) */
	if (optind < fr->argc) {
		/*
		 * Take the first positional argument, pending ones
		 * are just ignored.
		 */
		fr->font_filename = mystrdup(fr->argv[optind++]);
	}

	if (!fr->font_filename) {
		error("no input font file");
		exit(1);
	}

	if (!fr->atlas_filename)
		fr->atlas_filename = mystrdup("a.png");
	if (!fr->metrics_filename) {
		switch (fr->format) {
		case MF_TEXT:
			fr->metrics_filename = mystrdup("a.txt");
			break;
		case MF_BINARY:
			fr->metrics_filename = mystrdup("a.bin");
			break;
		}
	}

	if (!fr->atlas_width)
		fr->atlas_width = 256;
	if (!fr->atlas_height)
		fr->atlas_height = 256;
	if (!fr->pixel_height)
		fr->pixel_height = 16;

	if (!fr->ranges)
		get_ranges("33:126", fr);

	if (fr->option_verbose) {
		printf("input font file: %s\n", fr->font_filename);
		printf("output atlas file: %s\n", fr->atlas_filename);
		printf("output metrics file: %s\n", fr->metrics_filename);
		printf("antialised rendering: %s\n", fr->no_antialias ? "no" : "yes");
		printf("rendering size: %d\n", fr->pixel_height);
		printf("padding: %d\n", fr->padding);
		printf("border: %d\n", fr->border);
		const range_t *range = fr->ranges;
		for (; range; range = range->next)
			printf("rune range: %d to %d\n", range->lo, range->hi);
	}
}
