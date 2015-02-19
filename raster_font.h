#ifndef RASTER_FONT_H
#define RASTER_FONT_H

struct metrics_hdr {
	uint32_t glyph_count;
	float space_advance;
	uint32_t lut_offset;
	uint32_t glyph_offset;
};

struct glyph_def {
	float bearing[2];
	float advance[2];
	float size[2];
	uint16_t st0[2];
	uint16_t st1[2];
};

#endif /* RASTER_FONT_H */
