#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

struct bitmap {
	int width;
	int height;
	uint8_t *pixels;
};

void bitmap_alloc_pixels(struct bitmap *bitmap, int width, int height);
void bitmap_free_pixels(struct bitmap *bitmap);
struct bitmap *create_bitmap(int width, int height);
void destroy_bitmap(struct bitmap *bitmap);
uint8_t *bitmap_get_pixel(const struct bitmap *bitmap, int x, int y);
void bitmap_blit(struct bitmap *bp, const struct bitmap *src, int x, int y);
void bitmap_blit_ft_bitmap(struct bitmap *bp, const FT_Bitmap *ftbp, int x, int y);

#endif /* BITMAP_H */
