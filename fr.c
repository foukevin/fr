#include "fr.h"
#include "bitmap.h"
#include "raster_font.h"

#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H

struct glyph_metrics {
	float bearing[2];
	float advance[2];
	float size[2];

	double st0[2];
	double st1[2];
};

struct raster_glyph {
	struct raster_glyph *next;

	rune_t rune;
	struct bitmap bitmap;
	struct glyph_metrics metrics;
};

static const char *txt_hdr_fmt =
"space_advance=%f\n"
"glyph_count=%d\n";

static const char *txt_glyph_fmt =
"\n# Glyph %d\n"
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
	fprintf(fp, txt_glyph_fmt, i, glyph->rune,
		metrics->bearing[0], metrics->bearing[1],
		metrics->advance[0], metrics->advance[1],
		metrics->size[0], metrics->size[1],
		metrics->st0[0], metrics->st0[1],
		metrics->st1[0], metrics->st1[1]);
}

int write_metrics(const struct raster_glyph *glyph_list, int num_glyphs,
		  float space_advance, const char *path,
		  enum metrics_format format)
{
	const struct raster_glyph *glyph;
	struct metrics_hdr def;
	FILE *fp = NULL;

	fp = (format == MF_BINARY) ? fopen(path, "wb") : fopen(path, "w");
	if (!fp) {
		printf("error opening %s.\n", path);
		return 1;
	}

	switch (format) {
	case MF_TEXT:
		fprintf(fp, txt_hdr_fmt, space_advance, num_glyphs);
		int i = 0;
		for (glyph = glyph_list; glyph; glyph = glyph->next)
			write_text_glyph(fp, i++, glyph);
		break;
	case MF_BINARY:
		def.glyph_count = num_glyphs;
		def.space_advance = space_advance;
		def.lut_offset = sizeof(struct metrics_hdr);
		def.glyph_offset = def.lut_offset + sizeof(rune_t) * num_glyphs;
		fwrite(&def, sizeof(struct metrics_hdr), 1, fp);

		for (glyph = glyph_list; glyph; glyph = glyph->next)
			fwrite(&glyph->rune, sizeof(rune_t), 1, fp);
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
		printf("warning: not able to retrieve horizontal space advance");
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
	rune_t i;

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
			printf("warning: skipping rune U+%04X "
			       "(unable to load/render glyph)\n", i);
			continue;
		}

		if (!glyph_index) {
			printf("warning: skipping rune U+%04X "
			       "(glyph unavailable)\n", i);
			continue;
		}

		slot = face->glyph;
		int width = face->glyph->bitmap.width;
		int height = face->glyph->bitmap.rows;
		if (!width || !height) {
			printf("warning: skipping rune U+%04X "
			       "(zero width/height)\n", i);
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
int fill_atlas_and_metrics(struct bitmap *atlas, struct raster_glyph *glyph)
{
	int i;
	int pen_x = 0;
	int pen_y = 0;
	int line_height = 0;
	double atlas_scale[2] = {
		1.0 / (double)atlas->width,
		1.0 / (double)atlas->height
	};

	i = 0;
	while (glyph) {
		if (pen_x + glyph->bitmap.width > atlas->width) {
			/* Start a new line */
			pen_x = 0;
			pen_y += line_height + 1;
	 		line_height = 0;
			continue;
		}
		if (pen_y + glyph->bitmap.height > atlas->height) {
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

		pen_x += glyph->bitmap.width;
		glyph = glyph->next;
		i++;
	}

	return i;
};

int rasterize_font(FT_Face face, const struct options *opts)
{
	struct bitmap *atlas = NULL;
	struct raster_glyph *glyphs = NULL;
	int num_glyphs = 0;

	/*
	 * Raster all runes into individual bitmaps and gather metrics.
	 */
	for (const range_t *range = opts->ranges; range; range = range->next)
		rasterize_runes(face, &glyphs, &num_glyphs, range);

	/*
	 * Build the atlas texture from the rasterized glyphs and fill the
	 * texture coordinates.
	 */
	atlas = create_bitmap(opts->atlas_width, opts->atlas_height);
	fill_atlas_and_metrics(atlas, glyphs);

	/*
	 * Now the atlas has been filled and we know the glyph texture
	 * coordinates, we can proceed and write the files.
	 */
	write_atlas(atlas, opts->atlas_filename);
	write_metrics(glyphs, num_glyphs, space_advance(face),
		      opts->metrics_filename, opts->format);

	/* Free atlas and glyph list */
	destroy_bitmap(atlas);
	while (glyphs) {
		struct raster_glyph *next = glyphs->next;
		free(glyphs);
		glyphs = next;
	}

	if (opts->verbose)
		printf("%d glyphs rasterized to atlas\nDone.\n", num_glyphs);

	return 0;
}


