#ifndef FR_H
#define FR_H

#include "bitmap.h"
#include <stdint.h>

typedef struct rune_range {
	uint32_t lo;
	uint32_t hi;
	struct rune_range *next;
} range_t;

#define MF_TEXT (1)
#define	MF_BINARY (2)

struct fr {
	/* Options */
	char *atlas_filename;
	char *metrics_filename;
	char *font_filename;
	int option_verbose;
	int format;
	int atlas_width;
	int atlas_height;
	int pixel_height;
	int padding;
	range_t *ranges;

	/* State information */
	const char *progname;
	char **argv;
	int argc;
	int return_value;
};

void parse_options(struct fr *fr);
void rasterize_font(FT_Face face, const struct fr *fr);

#endif /* FR_H */
