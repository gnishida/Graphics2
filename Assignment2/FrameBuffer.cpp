#include "framebuffer.h"
#include <libtiff/tiffio.h>
#include <iostream>
#include "scene.h"
#include <math.h>
#include <algorithm>

using namespace std;

extern int rasterization_mode;

// makes an OpenGL window that supports SW, HW rendering, that can be displayed on screen
//        and that receives UI events, i.e. keyboard, mouse, etc.
FrameBuffer::FrameBuffer(int u0, int v0, int _w, int _h) : Fl_Gl_Window(u0, v0, _w, _h, 0) {
	w = _w;
	h = _h;
	pix = new unsigned int[w*h];
	zb  = new float[w*h];
}

FrameBuffer::~FrameBuffer() {
	delete [] pix;
}

// rendering callback; see header file comment
void FrameBuffer::draw() {
	// SW window, just transfer computed pixels from pix to HW for display
	glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, pix);
}

// function called automatically on event within window (callback)
int FrameBuffer::handle(int event)  {  
	switch(event) {   
	case FL_KEYBOARD:
		KeyboardHandle();
	case FL_PUSH:
		mousePushHandle();
	case FL_DRAG:
		mouseMoveHandle();
	default:
		break;
	}

	return 0;
}

void FrameBuffer::KeyboardHandle() {
	int key = Fl::event_key();

	switch (key) {
	case FL_Left:
		scene->currentPPC->TranslateLR(-1.0f);
		scene->Render();
		break;
	case FL_Right:
		scene->currentPPC->TranslateLR(1.0f);
		scene->Render();
		break;
	case FL_Up:
		scene->currentPPC->TranslateUD(1.0f);
		scene->Render();
		break;
	case FL_Down:
		scene->currentPPC->TranslateUD(-1.0f);
		scene->Render();
		break;
    case 'a':
		cerr << "pressed a" << endl;
		break;
	default:
		cerr << "INFO: do not understand keypress" << endl;
	}
}

void FrameBuffer::mousePushHandle() {
	make_current();
	lastPosition = V3(Fl::event_x(), Fl::event_y(), 0.0f);
}

void FrameBuffer::mouseMoveHandle() {
	if (Fl::event_state(FL_BUTTON1)) {
		float dx = Fl::event_x() - lastPosition.x();
		float dy = Fl::event_y() - lastPosition.y();

		scene->currentPPC->RotateAbout(scene->currentPPC->b, dx * 1.0f, V3(0.0f, 0.0f, 0.0f));
		scene->currentPPC->RotateAbout(scene->currentPPC->a, -dy * 1.0f, V3(0.0f, 0.0f, 0.0f));
	} else if (Fl::event_state(FL_BUTTON2)) {
		float dx = Fl::event_x() - lastPosition.x();
		float dy = Fl::event_y() - lastPosition.y();

		scene->currentPPC->TranslateLR(-dx);
		scene->currentPPC->TranslateUD(dy);
	} else if (Fl::event_state(FL_BUTTON3)) {
		float dy = Fl::event_y() - lastPosition.y();
		scene->currentPPC->TranslateFB(dy * 1.0f);
	}

	lastPosition = V3(Fl::event_x(), Fl::event_y(), 0.0f);
	scene->Render();
}

/**
 * Set all pixels to given color.
 *
 * @param bgr	the given color
 */
void FrameBuffer::Set(unsigned int bgr) {
	for (int uv = 0; uv < w*h; uv++) {
		pix[uv] = bgr;
	}
}

/**
 * Set one pixel to given color.
 * This function does not check neigher the range and the zbuffer.
 *
 * @param u		x coordinate of the pixel
 * @param v		y coordinate of the pixel
 * @param clr	the color
 */
void FrameBuffer::Set(int u, int v, unsigned int clr) {
	pix[(h-1-v)*w+u] = clr;
}

/**
 * Set one pixel to given color.
 * This function does not check the range, but check the zbuffer.
 *
 * @param u		x coordinate of the pixel
 * @param v		y coordinate of the pixel
 * @param clr	the color
 * @param z		z buffer
 */
void FrameBuffer::Set(int u, int v, unsigned int clr, float z) {
	if (zb[(h-1-v)*w+u] >= z) return;

	pix[(h-1-v)*w+u] = clr;
	zb[(h-1-v)*w+u] = z;
}

/**
 * Set one pixel to given color.
 * If the specified pixel is out of the screen, it does nothing.
 *
 * @param u		x coordinate of the pixel
 * @param v		y coordinate of the pixel
 * @param clr	the color
 */
void FrameBuffer::SetGuarded(int u, int v, unsigned int clr, float z) {
	if (u < 0 || u > w-1 || v < 0 || v > h-1) return;

	Set(u, v, clr, z);
}

// set all z values in SW ZB to z0
void FrameBuffer::SetZB(float z0) {
	for (int i = 0; i < w*h; i++) {
		zb[i] = z0;
	}
}

/**
 * Draw 2D segment with color interpolation.
 *
 * @param p0	the first point of the segment
 * @param c0	the color of the first point
 * @param p1	the second point of the segment
 * @param c1	the color of the second point
 */
void FrameBuffer::Draw2DSegment(const V3 &p0, const V3 &c0, const V3 &p1, const V3 &c1) {
	float dx = fabsf(p0.x() - p1.x());
	float dy = fabsf(p0.y() - p1.y());

	int n;
	if (dx < dy) {
		n = 1 + (int)dy;
	} else {
		n = 1 + (int)dx;
	}

	for (int i = 0; i <= n; i++) {
		float frac = (float) i / (float)n;
		V3 curr = p0 + (p1-p0) * frac;
		V3 currc = c0 + (c1-c0) * frac;
		int u = (int)curr[0];
		int v = (int)curr[1];
		SetGuarded(u, v, currc.GetColor(), curr[2]);
	}
}

/**
 * Draw 3D segment with color interpolation.
 *
 * @param ppc	the camera
 * @param p0	the first point of the segment
 * @param c0	the color of the first point
 * @param p1	the second point of the segment
 * @param c1	the color of the second point
 */
void FrameBuffer::Draw3DSegment(PPC* ppc, const V3 &p0, const V3 &c0, const V3 &p1, const V3 &c1) {
	V3 pp0, pp1;
	if (!ppc->Project(p0, pp0)) return;
	if (!ppc->Project(p1, pp1)) return;

	Draw2DSegment(pp0, c0, pp1, c1);
}

/**
 * Draw axis aligned rectangle.
 *
 * @param p0	the top left corner of the rectangle
 * @param p1	the bottom right corner of the rectangle
 * @param c		the color
 */
void FrameBuffer::DrawRectangle(const V3 &p0, const V3 &p1, const V3 &c) {
	V3 p2(p0.x(), p1.y(), 0.0f);
	V3 p3(p1.x(), p0.y(), 0.0f);

	Draw2DSegment(p0, c, p2, c);
	Draw2DSegment(p2, c, p1, c);
	Draw2DSegment(p1, c, p3, c);
	Draw2DSegment(p3, c, p0, c);
}

/**
 * Draw a frustum of the specified camera.
 *
 * @param ppc		the current camera
 * @param frustum	tthe camera to be drawn
 */
void FrameBuffer::DrawPPCFrustum(PPC* ppc, PPC* frustum) {
	//fDrawPPCFrustum(
}

/**
 * Load the frame buffer from the specified tiff file.
 *
 * @param filename		the tiff file name
 * @return				true if the load successes; false otherwise
 */
bool FrameBuffer::Load(char* filename) {
	TIFF* tiff = TIFFOpen(filename, "r");
	if (tiff == NULL) return false;

	int w2, h2;
	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w2);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h2);

	pix = (unsigned int*)_TIFFmalloc(sizeof(unsigned int) * w2 * h2);
	if (!TIFFReadRGBAImage(tiff, w, h, pix, 0)) {
		delete [] pix;
		pix = new unsigned int[w*h];
		TIFFClose(tiff);
		return false;
	}

	w = w2;
	h = h2;
	TIFFClose(tiff);

	return true;
}

/**
 * Save the frame buffer to a tiff file.
 *
 * @param filename		the file name to store the frame buffer
 * @return				true if the save successes; false otherwise
 */
bool FrameBuffer::Save(char* filename) {
	unsigned int *temp = new unsigned int[w * h];
	for (int v = 0; v < h; v++) {
		for (int u = 0; u < w; u++) {
			temp[v * w + u] = pix[(h - 1 - v) * w + u];
		}
	}

	TIFF* tiff = TIFFOpen(filename, "w");
	if (tiff == NULL) return false;

	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, sizeof(unsigned int));
	TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
	//TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	TIFFWriteEncodedStrip(tiff, 0, temp, w * h * sizeof(unsigned int));

	TIFFClose(tiff);

	delete [] temp;

	return true;
}

void FrameBuffer::Draw2DBigPoint(int u0, int v0, int psize, const V3 &color, float z) {
	for (int v = v0-psize/2; v <= v0+psize/2; v++) {
		for (int u = u0-psize/2; u <= u0+psize/2; u++) {
			SetGuarded(u, v, color.GetColor(), z);
		}
	}
}

void FrameBuffer::Draw3DBigPoint(PPC* ppc, const V3 &p, int psize, const V3 &color) {
	V3 pp;
	if (!ppc->Project(p, pp)) return;

	Draw2DBigPoint(pp.x(), pp.y(), psize, color, pp.z());
}

bool FrameBuffer::isHidden(int u, int v, float z) {
	if (zb[(h-1-v)*w+u] >= z) return true;
	else return false;
}

void FrameBuffer::rasterize(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2) {
	AABB box;

	// if the area is too small, skip this triangle.
	if (((p1.v - p0.v) ^ (p2.v - p0.v)).Length() < 1e-7) return;

	bool isFront = false;
	V3 pp0, pp1, pp2;
	if (!ppc->Project(p0.v, pp0)) return;
	if (!ppc->Project(p1.v, pp1)) return;
	if (!ppc->Project(p2.v, pp2)) return;

	// compute the bounding box
	box.AddPoint(pp0);
	box.AddPoint(pp1);
	box.AddPoint(pp2);

	// the bounding box should be inside the screen
	int u_min = (int)(box.minCorner().x() + 0.5f);
	if (u_min < 0) u_min = 0;;
	int u_max = (int)(box.maxCorner().x() - 0.5f);
	if (u_max >= w) u_max = w - 1;
	int v_min = (int)(box.minCorner().y() + 0.5f);
	if (v_min < 0) v_min = 0;
	int v_max = (int)(box.maxCorner().y() - 0.5f);
	if (v_max >= h) v_max = h - 1;

	// setup some variables
	float denom = ((pp1 - pp0) ^ (pp2 - pp0)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;

	// vertex colors that will be used only for Gouraud shading
	V3 c0 = scene->light->GetColor(ppc, p0.v, p0.c, p0.n);
	V3 c1 = scene->light->GetColor(ppc, p1.v, p1.c, p1.n);
	V3 c2 = scene->light->GetColor(ppc, p2.v, p2.c, p2.n);

	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - pp0) ^ (pp2 - pp0)).z() / denom;
			float t = -((pp - pp0) ^ (pp1 - pp0)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			if (scene->rasterization_mode == MODEL_SPACE_RASTERIZATION) {
				// project the point on the screen space.
				// if the point is behind the camera, skip this pixel.
				if (!ppc->Project(p, pp)) continue;
			} else {
				// interpolate the z coordinate
				pp[2] = pp0.z() * (1.0f - s - t) + pp1.z() * s + pp2.z() * t;

				// if the point is behind the camera, skip this pixel.
				if (pp.z() <= 0) continue;
			}

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;

			
			if (scene->rasterization_mode == MODEL_SPACE_RASTERIZATION) {
				s = s2;
				t = t2;
			}

			V3 c;

			if (scene->shading_mode == PHONG_SHADING) {
				// interpolate the color
				c = p0.c * (1.0f - s - t) + p1.c * s + p2.c * t;

				// interpolate the normal for Phong shading
				V3 n = p0.n * (1.0f - s - t) + p1.n * s + p2.n * t;
				c = scene->light->GetColor(ppc, p, c, n);
			} else if (scene->shading_mode == GOURAUD_SHADING) {
				// just interpolate the vertex colors
				c = c0 * (1.0f - s - t) + c1 * s + c2 * t;
			}

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c.GetColor(), pp.z());
		}
	}
}

void FrameBuffer::rasterizeWithTexture(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2, Texture* texture) {
	AABB box;

	// if the area is too small, skip this triangle.
	if (((p1.v - p0.v) ^ (p2.v - p0.v)).Length() < 1e-7) return;

	bool isFront = false;
	V3 pp0, pp1, pp2;
	if (!ppc->Project(p0.v, pp0)) return;
	if (!ppc->Project(p1.v, pp1)) return;
	if (!ppc->Project(p2.v, pp2)) return;

	// compute the bounding box
	box.AddPoint(pp0);
	box.AddPoint(pp1);
	box.AddPoint(pp2);

	// the bounding box should be inside the screen
	int u_min = (int)(box.minCorner().x() + 0.5f);
	if (u_min < 0) u_min = 0;;
	if (u_min > w) return;
	int u_max = (int)(box.maxCorner().x() - 0.5f);
	if (u_max >= w) u_max = w - 1;
	if (u_max < 0) return;
	int v_min = (int)(box.minCorner().y() + 0.5f);
	if (v_min < 0) v_min = 0;
	if (v_min > h) return;
	int v_max = (int)(box.maxCorner().y() - 0.5f);
	if (v_max >= h) v_max = h - 1;
	if (v_max < 0) return;

	// the bounding box of texture coordinate
	AABB boxTexCoord;
	boxTexCoord.AddPoint(V3(p0.t[0], p0.t[1], 0.0f));
	boxTexCoord.AddPoint(V3(p1.t[0], p1.t[1], 0.0f));
	boxTexCoord.AddPoint(V3(p2.t[0], p2.t[1], 0.0f));

	// set the mipmap according to the AABB
	texture->SetMipMap(u_max - u_min, v_max - v_min, boxTexCoord.maxCorner().x() - boxTexCoord.minCorner().x(), boxTexCoord.maxCorner().y() - boxTexCoord.minCorner().y());

	// setup some variables
	float denom = ((pp1 - pp0) ^ (pp2 - pp0)).z();
	M33 Q;
	Q.SetColumn(0, p0.v - ppc->C);
	Q.SetColumn(1, p1.v - ppc->C);
	Q.SetColumn(2, p2.v - ppc->C);
	Q = Q.Inverted() * camMat;



	for (int u = u_min; u <= u_max; u++) {
		for (int v = v_min; v <= v_max; v++) {
			V3 pp((u + 0.5f) * ppc->a.Length(), (v + 0.5f) * ppc->b.Length(), 0.0f);

			float s = ((pp - pp0) ^ (pp2 - pp0)).z() / denom;
			float t = -((pp - pp0) ^ (pp1 - pp0)).z() / denom;

			// if the point is outside the triangle, skip it.
			if (s < 0 || s > 1 || t < 0 || t > 1 || s + t > 1) continue;
				
			V3 a = Q * V3((float)u + 0.5f, (float)v + 0.5f, 1.0f);
			float w2 = a.x() + a.y() + a.z();
			float s2 = a.y() / w2;
			float t2 = a.z() / w2;

			// unproject and locate the corresponding point on the triangle plane.
			V3 p = p0.v * (1 - s2 - t2) + p1.v * s2 + p2.v * t2;

			if (scene->rasterization_mode == MODEL_SPACE_RASTERIZATION) {
				// project the point on the screen space.
				// if the point is behind the camera, skip this pixel.
				if (!ppc->Project(p, pp)) continue;
			} else {
				// interpolate the z coordinate
				pp[2] = pp0.z() * (1.0f - s - t) + pp1.z() * s + pp2.z() * t;

				// if the point is behind the camera, skip this pixel.
				if (pp.z() <= 0) continue;
			}

			// check if the point is occluded by other triangles.
			if (zb[(h-1-v)*w+u] >= pp.z()) continue;

			if (scene->rasterization_mode == MODEL_SPACE_RASTERIZATION) {
				s = s2;
				t = t2;
			}



			// get the corresponding color by using bi-linear interpolation lookup
			float t_x = p0.t[0] * (1.0f - s - t) + p1.t[0] * s + p2.t[0] * t;
			float t_y = p0.t[1] * (1.0f - s - t) + p1.t[1] * s + p2.t[1] * t;



			V3 c = texture->GetColor(t_x, t_y);

			// draw the pixel (u,v) with the interpolated color.
			Set(u, v, c.GetColor(), pp.z());
		}
	}

}
