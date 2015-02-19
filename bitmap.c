#include "bitmap.h"

void bitmap_alloc_pixels(struct bitmap *bitmap, int width, int height)
{
	bitmap->width = width;
	bitmap->height = height;
	bitmap->pixels = calloc(sizeof(uint8_t), width * height);
}

void bitmap_free_pixels(struct bitmap *bitmap)
{
	if (bitmap->pixels) {
		free(bitmap->pixels);
		bitmap->pixels = NULL;
	}
}

struct bitmap *create_bitmap(int width, int height)
{
	struct bitmap *bitmap = malloc(sizeof(struct bitmap));
	bitmap_alloc_pixels(bitmap, width, height);
	return bitmap;
}

void destroy_bitmap(struct bitmap *bitmap)
{
	bitmap_free_pixels(bitmap);
	free(bitmap);
}

uint8_t *bitmap_get_pixel(const struct bitmap *bitmap, int x, int y)
{
	return bitmap->pixels + bitmap->width * y + x;
}

void bitmap_blit(struct bitmap *bp, const struct bitmap *src, int x, int y)
{
	int row;
	for (row = 0; row < src->height; ++row) {
		memcpy(&bp->pixels[(y + row) * bp->width + x],
		       &src->pixels[row * src->width],
		       sizeof(uint8_t) * src->width);
	}
}

void bitmap_blit_ft_bitmap(struct bitmap *bp, const FT_Bitmap *ftbp, int x, int y)
{
	unsigned int row;
	int width = ftbp->width;
	int height = ftbp->rows;

	if ((x + width) > bp->width || (y + height) > bp->height)
		return;

	for (row = 0; row < ftbp->rows; ++row, ++y) {
        	memcpy(bitmap_get_pixel(bp, x, y), &ftbp->buffer[row * width],
		       sizeof(uint8_t) * width);
	}
}
