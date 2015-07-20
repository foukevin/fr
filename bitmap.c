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

static inline uint8_t get_pixel_value(const FT_Bitmap *ft_bitmap, int x, int y)
{
	uint8_t byte = ft_bitmap->buffer[(ft_bitmap->pitch * y) + (x / 8)];
	uint8_t bit_mask = 0x80 >> (x % 8);
	return (byte & bit_mask) ? 255 : 0;
}

void bitmap_blit_ft_bitmap(struct bitmap *bitmap, const FT_Bitmap *ft_bitmap,
			   int x, int y)
{
	unsigned int row;
	int width = ft_bitmap->width;
	int height = ft_bitmap->rows;

	if ((x + width) > bitmap->width || (y + height) > bitmap->height)
		return;

	switch (ft_bitmap->pixel_mode) {
	case FT_PIXEL_MODE_GRAY:
		for (row = 0; row < ft_bitmap->rows; ++row, ++y) {
			memcpy(bitmap_get_pixel(bitmap, x, y),
			       &ft_bitmap->buffer[row * width],
			       sizeof(uint8_t) * width);
		}
		break;
	case FT_PIXEL_MODE_MONO:
		printf("pitch: %d\n", ft_bitmap->pitch);
		for (row = 0; row < height; ++row, ++y) {
			int i;
			for (i = 0; i < width; ++i) {
				uint8_t *pixel = bitmap_get_pixel(bitmap, i, y);
				*pixel = get_pixel_value(ft_bitmap, i, y);
			}
		}
		break;
	}
}
