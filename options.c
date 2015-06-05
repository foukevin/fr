#include "fr.h"
#include "error.h"

#include <getopt.h>
#include <stddef.h> /* NULL */
#include <stdlib.h> /* exit */
#include <string.h>

static struct options opt;
static const char *version = "0.1";

static char *mystrdup(const char *s)
{
	char *d;
	int len = strlen(s);
	d = malloc(len + 1);
	if (!d)
		die("out of memory");

	strncpy(d, s, len);
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
	printf("  --metrics-format=[text|binary]\n"
	       "                           Write metrics as text or binary\n");
	printf("  --rune=,<range>          Comma separated unicode point or point ranges\n");
	printf("Notes:\n");
	printf("  Ranges are in the form <c>, <l>:<u> or <l>+<n>; "
	       "of single code point <c>, lower bound <l>, upper bound <u> and extend <n>\n");
}

static struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
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
 * Sets the glyph metrics format of the option struct.
 * Returns 0 (no error) or 1 whether the format is valid or not.
 */
static int get_format(const char *s)
{
	if (!strcmp(s, "text"))
		opt.format = MF_TEXT;
	else if (!strcmp(s, "binary"))
		opt.format = MF_BINARY;
	else
		return 1;

	return 0;
}

static int get_ranges(const char *s)
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
			range->next = opt.ranges;
			opt.ranges = range;
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

struct options *parse_options(struct fr *fr)
{
	int invalid_arg = 0;

	while (1) {
		char c = getopt_long(fr->argc, fr->argv, "hvo:m:W:H:s:p:f:",
				     long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(fr);
			exit(0);
			break;
		case 'v':
			opt.verbose = 1;
			break;
		case 'o':
			opt.atlas_filename = mystrdup(optarg);
			break;
		case 'm':
			opt.metrics_filename = mystrdup(optarg);
			break;
		case 'W':
			opt.atlas_width = atoi(optarg);
			if (opt.atlas_width <= 0) {
				error("invalid atlas width: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'H':
			opt.atlas_height = atoi(optarg);
			if (opt.atlas_height <= 0) {
				error("invalid atlas height: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 's':
			opt.pixel_height = atoi(optarg);
			if (opt.pixel_height <= 0) {
				error("invalid size: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'p':
			opt.padding = atoi(optarg);
			if (opt.padding < 0) {
				error("invalid padding: %s", optarg);
				invalid_arg = 1;
			}
			break;
		case 'r':
			get_ranges(optarg);
			break;
		case 'f':
			if (get_format(optarg)) {
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
			return NULL;
	}

	/* Handle non-option arguments (ie: font names) */
	if (optind < fr->argc) {
		/*
		 * Take the first positional argument, pending ones
		 * are just ignored.
		 */
		opt.font_filename = mystrdup(fr->argv[optind++]);
	}

	if (!opt.font_filename) {
		error("no input font file");
		return NULL;
	}

	if (!opt.atlas_filename)
		opt.atlas_filename = mystrdup("a.png");
	if (!opt.metrics_filename) {
		switch (opt.format) {
		case MF_TEXT:
			opt.metrics_filename = mystrdup("a.txt");
			break;
		case MF_BINARY:
			opt.metrics_filename = mystrdup("a.bin");
			break;
		}
	}

	if (!opt.atlas_width)
		opt.atlas_width = 256;
	if (!opt.atlas_height)
		opt.atlas_height = 256;
	if (!opt.pixel_height)
		opt.pixel_height = 16;

	if (!opt.ranges)
		get_ranges("33:126");

	if (opt.verbose) {
		printf("input font file: %s\n", opt.font_filename);
		printf("output atlas file: %s\n", opt.atlas_filename);
		printf("output metrics file: %s\n", opt.metrics_filename);
		printf("size: %d\n", opt.pixel_height);
		const range_t *range = opt.ranges;
		for (; range; range = range->next)
			printf("rune range: %d to %d\n", range->lo, range->hi);
	}
	return &opt;
}

void destroy_options(struct options *opts)
{
	if (!opts)
		return;

	if (opts->atlas_filename) {
		free(opts->atlas_filename);
		opts->atlas_filename = NULL;
	}
	if (opts->metrics_filename) {
		free(opts->metrics_filename);
		opts->metrics_filename = NULL;
	}
	if (opts->font_filename) {
		free(opts->font_filename);
		opts->font_filename = NULL;
	}

	range_t *range = opts->ranges;
	while (range) {
		range_t *next = range->next;
		free(range);
		range = next;
	}
	opts->ranges = NULL;
}
