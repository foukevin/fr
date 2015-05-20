#ifndef FR_H
#define FR_H

#include "bitmap.h"
#include <stdint.h>

typedef struct rune_range {
	uint32_t lo;
	uint32_t hi;
	struct rune_range *next;
} range_t;

enum metrics_format {
	MF_TEXT,
	MF_BINARY,
};

struct options {
	char *atlas_filename;
	char *metrics_filename;
	char *font_filename;
	int verbose;
	enum metrics_format format;
	int atlas_width;
	int atlas_height;
	int pixel_height;
	int padding;
	range_t *ranges;
};

struct options *parse_options(int argc, char **argv);
void destroy_options(struct options *opts);
int rasterize_font(FT_Face face, const struct options *opts);

#endif /* FR_H */
