#include "fr.h"
#include "bitmap.h"
#include "raster_font.h"
#include "error.h"

#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H

/* struct holding a glyph metrics */
struct glyph_metrics {
	float bearing[2];
	float advance[2];
	float size[2];

	double st0[2];
	double st1[2];
};

struct raster_glyph {
	struct raster_glyph *next;

	uint32_t rune;
	struct bitmap bitmap;
	struct glyph_metrics metrics;
};

static const char *txt_hdr_fmt =
"space_advance=%f\n"
"glyph_count=%d\n";

static const char *txt_glyph_fmt =
"\n# Glyph %d (%s)\n"
"rune=%d\n"
"horizontal_bearing=%f\n"
"vertical_bearing=%f\n"
"horizontal_advance=%f\n"
"vertical_advance=%f\n"
"width=%f\n"
"height=%f\n"
"s0=%f\n"
"t0=%f\n"
"s1=%f\n"
"t1=%f\n";

static FT_Int32 load_flags =  FT_LOAD_DEFAULT;

int main(int argc, char **argv)
{
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

	fr->argc = argc;
	fr->argv = argv;

	parse_options(fr);

	error = FT_Init_FreeType(&library);
	if (error)
		die("unable to initialize FreeType");

	error = FT_New_Face(library, fr->font_filename, 0, &face);
	if (error)
		die("unable to load font %s", fr->font_filename);

	error = FT_Set_Pixel_Sizes(face, 0, fr->pixel_height);
	if (error)
		die("unable to set font size");

	rasterize_font(face, fr);

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	/* clean up */
	if (fr->atlas_filename) {
		free(fr->atlas_filename);
		fr->atlas_filename = NULL;
	}
	if (fr->metrics_filename) {
		free(fr->metrics_filename);
		fr->metrics_filename = NULL;
	}
	if (fr->font_filename) {
		free(fr->font_filename);
		fr->font_filename = NULL;
	}

	range_t *range = fr->ranges;
	while (range) {
		range_t *next = range->next;
		free(range);
		range = next;
	}
	fr->ranges = NULL;

	return fr->return_value;
}


void write_binary_glyph(FILE *fp, const struct glyph_metrics *metrics)
{
	struct glyph_def m;

	m.bearing[0] = metrics->bearing[0];
	m.bearing[1] = metrics->bearing[1];
	m.advance[0] = metrics->advance[0];
	m.advance[1] = metrics->advance[1];
	m.size[0] = metrics->size[0];
	m.size[1] = metrics->size[1];
	m.st0[0] = metrics->st0[0] * (double)UINT16_MAX;
	m.st0[1] = metrics->st0[1] * (double)UINT16_MAX;
	m.st1[0] = metrics->st1[0] * (double)UINT16_MAX;
	m.st1[1] = metrics->st1[1] * (double)UINT16_MAX;

	fwrite(&m, sizeof(m), 1, fp);
}

void write_text_glyph(FILE *fp, int i, const struct raster_glyph *glyph)
{
	const struct glyph_metrics *metrics;
	metrics = &glyph->metrics;

	/* Encode unicode code point into utf8 stream */
	uint32_t rune = glyph->rune;
	char utf8[5];
	if (rune < 0x80) {
		/* 1 byte encoding */
		utf8[0] = (uint8_t)rune;
		utf8[1] = '\0';
	} else if (rune < 0x800) {
		/* 2 bytes encoding */
		utf8[0] = 0xC0 | (rune >> 6);
		utf8[1] = 0x80 | (rune & 0x3F);
		utf8[2] = '\0';
	} else if (rune < 0x10000) {
		/* 3 bytes encoding */
		utf8[0] = 0xE0 | (rune >> 12);
		utf8[1] = 0x80 | ((rune >> 6) & 0x3F);
		utf8[2] = 0x80 | (rune & 0x3F);
		utf8[3] = '\0';
	} else if (rune < 0x200000) {
		/* 4 bytes encoding */
		utf8[0] = 0xF0 | (rune >> 24);
		utf8[1] = 0x80 | ((rune >> 12) & 0x3F);
		utf8[2] = 0x80 | ((rune >> 6) & 0x3F);
		utf8[3] = 0x80 | (rune & 0x3F);
		utf8[4] = '\0';
	} else {
		/* not supported yet */
		utf8[0] = '\0';
	}

	fprintf(fp, txt_glyph_fmt, i, utf8, glyph->rune,
		metrics->bearing[0], metrics->bearing[1],
		metrics->advance[0], metrics->advance[1],
		metrics->size[0], metrics->size[1],
		metrics->st0[0], metrics->st0[1],
		metrics->st1[0], metrics->st1[1]);
}

int write_metrics(const struct raster_glyph *glyph_list, int num_glyphs,
		  float space_advance, const char *path, int format)
{
	const struct raster_glyph *glyph;
	struct metrics_hdr def;
	FILE *fp = NULL;

	fp = fopen(path, format == MF_BINARY ? "wb" : "w");
	if (!fp) {
		error("opening %s", path);
		return 1;
	}

	switch (format) {
	case MF_TEXT:
		fprintf(fp, txt_hdr_fmt, space_advance, num_glyphs);
		int i = 0;
		glyph = glyph_list;
		while (glyph) {
			write_text_glyph(fp, i, glyph);
			++i;
			glyph = glyph->next;
		}
		break;
	case MF_BINARY:
		def.glyph_count = num_glyphs;
		def.space_advance = space_advance;
		def.lut_offset = sizeof(struct metrics_hdr);
		def.glyph_offset = def.lut_offset + sizeof(uint32_t) * num_glyphs;
		fwrite(&def, sizeof(struct metrics_hdr), 1, fp);

		for (glyph = glyph_list; glyph; glyph = glyph->next)
			fwrite(&glyph->rune, sizeof(uint32_t), 1, fp);
		for (glyph = glyph_list; glyph; glyph = glyph->next)
			write_binary_glyph(fp, &glyph->metrics);
		break;
	}

	fclose(fp);
	return 0;
}

int write_atlas(const struct bitmap *bp, const char *file_name)
{
	FILE *fp = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_byte ** row_pointers = NULL;
	int x, y;

	const int pixel_size = sizeof(uint8_t);

	fp = fopen(file_name, "wb");
	if (!fp)
		return 1;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
       		return 1;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, NULL);
		return 1;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
        	goto png_failure;

	/* Set image attributes */
	png_set_IHDR(png_ptr,
		     info_ptr,
		     bp->width,
		     bp->height,
		     8, // Depth, bpp
		     PNG_COLOR_TYPE_GRAY, //PNG_COLOR_TYPE_RGB,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT,
		     PNG_FILTER_TYPE_DEFAULT);

	/* Initialize rows of PNG */
	row_pointers = png_malloc(png_ptr, bp->height * sizeof(png_byte *));
	for (y = 0; y < bp->height; ++y) {
		png_byte *row =	png_malloc(png_ptr, sizeof(uint8_t) * bp->width * pixel_size);
		row_pointers[y] = row;
		for (x = 0; x < bp->width; ++x)
			*row++ = *bitmap_get_pixel(bp, x, y);
	}

	/* Write the image data to "fp" */
	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	fclose(fp);

	for (y = 0; y < bp->height; y++)
		png_free(png_ptr, row_pointers[y]);
	png_free(png_ptr, row_pointers);

	return 0;

png_failure:
	png_destroy_write_struct (&png_ptr, &info_ptr);
	return 1;
}

float space_advance(FT_Face face)
{
	float advance;
	FT_UInt glyph_index;
	FT_GlyphSlot slot;

	glyph_index = FT_Get_Char_Index(face, ' ');
	if (FT_Load_Glyph(face, glyph_index, load_flags)) {
		warning("unaible to retrieve horizontal space advance");
		advance = 0.0f;
	} else {
		slot = face->glyph;
		advance = (float)slot->metrics.horiAdvance / 65535.0f;
	}

	return advance;
}

int rasterize_runes(FT_Face face, struct raster_glyph **head, int *num_glyphs,
		    const range_t *range)
{
	FT_UInt glyph_index;
	FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;
	FT_GlyphSlot slot;
	uint32_t i;

	/*
	 * Because the raster_glyph list is single-linked, every insertion
	 * is done at the front. Therefore runes are processed in reverse
	 * order so that later steps using the list have the 'right' order.
	 * Not that it is *that* important but the resulting atlas texture
	 * won't look awkward.
	 */
	for (i = range->hi; i >= range->lo; i--) {
		glyph_index = FT_Get_Char_Index(face, i);

		if (FT_Load_Glyph(face, glyph_index, load_flags)
		    || FT_Render_Glyph(face->glyph, render_mode)) {
			warning("skipping rune U+%04X (unable to load/render glyph)", i);
			continue;
		}

		if (!glyph_index) {
			warning("skipping rune U+%04X (glyph unavailable)", i);
			continue;
		}

		slot = face->glyph;
		int width = face->glyph->bitmap.width;
		int height = face->glyph->bitmap.rows;
		if (!width || !height) {
			warning("skipping rune U+%04X (zero width/height)", i);
			continue;
		}

		struct raster_glyph *glyph = malloc(sizeof(*glyph));
		bitmap_alloc_pixels(&glyph->bitmap, width, height);
		bitmap_blit_ft_bitmap(&glyph->bitmap, &face->glyph->bitmap, 0, 0);

		glyph->rune = i;
		struct glyph_metrics *metrics = &glyph->metrics;
		metrics->advance[0] = (float)slot->metrics.horiAdvance / 65535.0f;
		metrics->advance[1] = (float)slot->metrics.vertAdvance / 65535.0f;
		metrics->bearing[0] = slot->metrics.horiBearingX / 65535.0f;
		metrics->bearing[1] = slot->metrics.horiBearingY / 65535.0f;
		metrics->size[0] = slot->metrics.width / 65535.0f;
		metrics->size[1] = slot->metrics.height / 65535.0f;

		/* Advance next glyph */
		glyph->next = *head;
		*head = glyph;
		(*num_glyphs)++;
	}

	return 0;
}

/* Return numbers of glyphs actually in the atlas */
int fill_atlas_and_metrics(struct bitmap *atlas, struct raster_glyph *glyph,
			   int padding)
{
	int i;
	int pen_x = padding;
	int pen_y = padding;
	int line_height = 0;
	double atlas_scale[2] = {
		1.0 / (double)atlas->width,
		1.0 / (double)atlas->height
	};

	i = 0;
	while (glyph) {
		if (pen_x + glyph->bitmap.width + padding > atlas->width) {
			/* Start a new line */
			pen_x = padding;
			pen_y += line_height + padding;
	 		line_height = 0;
			continue;
		}
		if (pen_y + glyph->bitmap.height + padding > atlas->height) {
			/* Bitmap is too small */
			break;
		}

		if (glyph->bitmap.height > line_height)
			line_height = glyph->bitmap.height;

		/* Blit the glyph into the atlas and free its pixels. */
		bitmap_blit(atlas, &glyph->bitmap, pen_x, pen_y);
		bitmap_free_pixels(&glyph->bitmap);

		/* Build texture coordinates according to atlas scale. */
		double st0[2];
		double st1[2];

		struct glyph_metrics *metrics = &glyph->metrics;
		st0[0] = (double)pen_x;
		st0[1] = (double)pen_y;
		st1[0] = (double)(pen_x + glyph->bitmap.width);
		st1[1] = (double)(pen_y + glyph->bitmap.height);

		metrics->st0[0] = st0[0] * atlas_scale[0];
		metrics->st0[1] = st0[1] * atlas_scale[1];
		metrics->st1[0] = st1[0] * atlas_scale[0];
		metrics->st1[1] = st1[1] * atlas_scale[1];

#if 0
		printf("glyph index:%d, rune:%04x\n", i, glyph->rune);
		printf("advance (%f, %f) ", metrics->advance[0], metrics->advance[1]);
		printf("bearing (%f, %f) ", metrics->bearing[0], metrics->bearing[1]);
		printf("size (%f, %f)\n", metrics->size[0], metrics->size[1]);
		printf("(float) st0 (%f, %f), st1 (%f, %f)\n", s0, t0, s1, t1);
		printf("(uint16_t) st0 (%04x, %04x), st1 (%04x, %04x)\n",
		       metrics->s0, metrics->t0, metrics->s1, metrics->t1);
#endif

		pen_x += glyph->bitmap.width + padding;
		glyph = glyph->next;
		i++;
	}

	return i;
};

void rasterize_font(FT_Face face, const struct fr *fr)
{
	struct bitmap *atlas = NULL;
	struct raster_glyph *glyphs = NULL;
	int num_glyphs = 0;

	/*
	 * Raster all runes into individual bitmaps and gather metrics.
	 */
	const range_t *range;
	for (range = fr->ranges; range; range = range->next)
		rasterize_runes(face, &glyphs, &num_glyphs, range);

	/*
	 * Build the atlas texture from the rasterized glyphs and fill the
	 * texture coordinates.
	 */
	atlas = create_bitmap(fr->atlas_width, fr->atlas_height);
	fill_atlas_and_metrics(atlas, glyphs, fr->padding);

	/*
	 * Now the atlas has been filled and we know the glyph texture
	 * coordinates, we can proceed and write the files.
	 */
	write_atlas(atlas, fr->atlas_filename);
	write_metrics(glyphs, num_glyphs, space_advance(face),
		      fr->metrics_filename, fr->format);

	/* Free atlas and glyph list */
	destroy_bitmap(atlas);
	while (glyphs) {
		struct raster_glyph *next = glyphs->next;
		free(glyphs);
		glyphs = next;
	}

	if (fr->option_verbose)
		printf("%d glyphs rasterized to atlas\nDone.\n", num_glyphs);
}
