#include "fr.h"
#include "error.h"

#include <ft2build.h>
#include FT_FREETYPE_H

int main(int argc, char **argv)
{
	int status = 0;
	struct fr *fr, fr_storage;

	FT_Error error;
	FT_Library library;
	FT_Face face;

	ERROR_INIT;

	fr = &fr_storage;
	memset(fr, 0, sizeof(*fr));

	if (*argv == NULL) {
		fr->progname = "fr";
	} else {
		fr->progname = *argv;
	}

	struct options *opts = parse_options(argc, argv, fr);
	if (!opts) {
		destroy_options(opts);
		return 1;
	}

	error = FT_Init_FreeType(&library);
	if (error)
		die("unable to initialize FreeType");

	error = FT_New_Face(library, opts->font_filename, 0, &face);
	if (error)
		die("unable to load font %s", opts->font_filename);

	error = FT_Set_Pixel_Sizes(face, 0, opts->pixel_height);
	if (error)
		die("unable to set font size");

	status = rasterize_font(face, opts);

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	destroy_options(opts);
	return status;
}
