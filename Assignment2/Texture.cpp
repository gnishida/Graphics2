#include "Texture.h"
#include <libtiff/tiffio.h>
#include <assert.h>
#include <iostream>

Texture::Texture(const char* filename) {
	TIFF* tiff = TIFFOpen(filename, "r");
	if (tiff == NULL) throw "File is not accessible.";

	int w, h;

	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);

	unsigned int* image = (unsigned int*)_TIFFmalloc(sizeof(unsigned int) * w * h);
	widths.push_back(w);
	heights.push_back(h);
	images.push_back(image);
	if (!TIFFReadRGBAImage(tiff, w, h, image, 0)) {
		delete [] image;
		image = NULL;
		TIFFClose(tiff);
		return;
	}

	TIFFClose(tiff);

	CreateMipMap(w, h);

	mipmap_id1 = 0;
	mipmap_id2 = 0;
	mipmap_s = 1.0f;
}

Texture::~Texture() {
	for (int i = 0; i < images.size(); i++) {
		delete [] images[i];
	}
}

/**
 * Get the color at the specified texel (s, t) of this image.
 *
 * @param u		the x coordinate (0.0 - 1.0)
 * @param v		the y coordinate (0.0 - 1.0)
 * @return		the color
 */
V3 Texture::GetColor(float s, float t) {
	if (images.size() == 0) return V3(0.0f, 0.0f, 0.0f);

	// tri-linear interpolation
	V3 c1 = GetColor(images[mipmap_id1], widths[mipmap_id1], heights[mipmap_id1], s, t);
	V3 c2 = GetColor(images[mipmap_id2], widths[mipmap_id2], heights[mipmap_id2], s, t);

	return c1 * (1.0f - mipmap_s) + c2 * mipmap_s;
}

/**
 * Find the nearest two mipmaps according to the width/height of the AABB.
 *
 * @param width		the width of the AABB
 * @param height	the height of the AABB
 * @param ds		the difference between the min and max of the s coordinates of the triangle
 * @param dt		the difference between the min and max of the t coordinates of the triangle
 */
void Texture::SetMipMap(int width, int height, float ds, float dt) {
	if (ds == 0.0f || dt == 0.0f) {
		mipmap_id1 = 0;
		mipmap_id2 = 0;
		return;
	}

	int w = (int)(width / ds) + 1;
	int h = (int)(height / dt) + 1;

	for (mipmap_id2 = 0; mipmap_id2 < images.size(); mipmap_id2++) {
		if (w > widths[mipmap_id2]) {
			if (mipmap_id2 == 0) {
				mipmap_id1 = 0;
				mipmap_s = 1.0f;
			} else {
				mipmap_id1 = mipmap_id2 - 1;
				mipmap_s = (float)(widths[mipmap_id1] - w) / (float)(widths[mipmap_id1] - widths[mipmap_id2]);
			}
			break;
		}
		if (h > heights[mipmap_id2]) {
			if (mipmap_id2 == 0) {
				mipmap_id1 = 0;
				mipmap_s = 1.0f;
			} else {
				mipmap_id1 = mipmap_id2 - 1;
				mipmap_s = (float)(heights[mipmap_id1] - h) / (float)(heights[mipmap_id1] - heights[mipmap_id2]);
			}
			break;
		}
	}

	if (mipmap_id2 >= images.size()) {
		mipmap_s = 1.0f;
		mipmap_id2 = images.size() - 1;
		mipmap_id1 = mipmap_id2;
	}
}

/**
 * Get the color at the texel (u, v) in the specified mipmap image.
 *
 * @param image		the mipmap image
 * @param width		the width of the mipmap image
 * @param height	the height of the mipmap image
 * @param u			the x coordinate of the texel
 * @param v			the y coordinate of the texel
 * @return			the color
 */
V3 Texture::GetColor(unsigned int* image, int width, int height, float u, float v) {
	// locate the corresponding (x, y) in the mipmap image
	float x = (float)(u - (int)u) * (float)width;
	float y = (float)(v - (int)v) * (float)height;
	if (x < 0) x += width;
	if (y < 0) y += height;

	// locte the surrounding 4 texels
	int x0, y0, x1, y1;
	float s, t;
	if (x < 0.5f) {
		x0 = 0;
		s = 1.0f;
	} else {
		x0 = (int)(x - 0.5f);
		s = x - (float)x0 - 0.5f;
	}
	if (y < 0.5f) {
		y0 = 0;
		t = 1.0f;
	} else {
		y0 = (int)(y - 0.5f);
		t = y - (float)y0 - 0.5f;
	}

	x1 = x0 + 1;
	y1 = y0 + 1;
	if (x1 >= width) x1 = width - 1;
	if (y1 >= height) y1 -= height - 1;
	
	// get the colors of 4 texels
	V3 c0, c1, c2, c3;
	c0.SetColor(image[x0 + y0 * width]);
	c1.SetColor(image[x1 + y0 * width]);
	c2.SetColor(image[x1 + y1 * width]);
	c3.SetColor(image[x0 + y1 * width]);
	
	return c0 * (1 - s) * (1 - t) + c1 * s * (1 - t) + c2 * s * t  + c3 * (1 - s) * t;
}

/**
 * Create mipmap images.
 *
 * @param width		the width of the original image
 * @param height	the height of the original image
 */
void Texture::CreateMipMap(int width, int height) {
	V3 c0, c1, c2, c3;

	int w = width / 2;
	int h = height / 2;

	while (w >= 1 && h >= 1) {
		unsigned int* image = (unsigned int*)_TIFFmalloc(sizeof(unsigned int) * w * h);
		widths.push_back(w);
		heights.push_back(h);
		images.push_back(image);

		float scaleX = (float)width / (float)w;
		float scaleY = (float)height / (float)h;

		for (int i = 0; i< h; i++) {
			for (int j = 0; j < w; j++) {
				// locate (x, y) in the original image
				float x = ((float)j + 0.5f) * scaleX;
				float y = ((float)i + 0.5f) * scaleY;

				// locate surrounding 4 texels
				int x0, y0, x1, y1;
				float s, t;

				if (x < 0.5f) {
					x0 = 0;
					s = 1.0f;
				} else {
					x0 = (int)(x - 0.5f);
					s = x - (float)x0 - 0.5f;
				}
				if (y < 0.5f) {
					y0 = 0;
					t = 1.0f;
				} else {
					y0 = (int)(y - 0.5f);
					t = y - (float)y0 - 0.5f;
				}

				x1 = x0 + 1;
				y1 = y0 + 1;
				if (x1 >= width) x1 = width - 1;
				if (y1 >= height) y1 = height - 1;

				// get the colors of 4 texels
				c0.SetColor(images[0][x0 + y0 * width]);
				c1.SetColor(images[0][x1 + y0 * width]);
				c2.SetColor(images[0][x1 + y1 * width]);
				c3.SetColor(images[0][x0 + y1 * width]);

				// bi-linear interpolation
				V3 c = c0 * (1.0f - s) * (1.0f - t) + c1 * s * (1.0f - t) + c2 * s * t + c3 * (1.0f - s) * t;

				image[j + i * w] = c.GetColor();
			}
		}

		w /= 2;
		h /= 2;
	}
}
