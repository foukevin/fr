#include "fr.h"

#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int main(int argc, char **argv)
{
	int status = 0;

	FT_Error error;
	FT_Library library;
	FT_Face face;

	struct options *opts = parse_options(argc, argv);
	if (!opts) {
		status = 1;
		goto arg_error;
	}

	error = FT_Init_FreeType(&library);
	if (error) {
		printf("internal error.\n");
		status = 1;
		goto lib_failure;
	}

	error = FT_New_Face(library, opts->font_filename, 0, &face);
        if (error == FT_Err_Unknown_File_Format) {
		printf("unsupported font file format.\n");
		status = 1;
		goto face_failure;
        }

	error = FT_Set_Pixel_Sizes(face, 0, opts->pixel_height);
	if (error) {
		printf("error setting size.\n");
		status = 1;
		goto size_failure;
	}

	status = rasterize_font(face, opts);

size_failure:
	FT_Done_Face(face);
face_failure:
	FT_Done_FreeType(library);
lib_failure:
arg_error:
	destroy_options(opts);
	return status;
}
