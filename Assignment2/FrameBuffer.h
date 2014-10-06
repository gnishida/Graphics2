#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <GL/glut.h>
#include "V3.h"
#include "M33.h"
#include "TMesh.h"
#include "PPC.h"
#include "Texture.h"

// framebuffer + window class

class FrameBuffer : public Fl_Gl_Window {
public:
	/** software color buffer (The first pixel is the bottom left corner.) */
	unsigned int *pix;

	/** software Z buffer */
	float *zb;

	/** image wdith resolution */
	int w;
		
	/** image height resolution */
	int h;

	/** the last position of the mouse pointer */
	V3 lastPosition;

public:
	FrameBuffer(int u0, int v0, int _w, int _h); // constructor, top left coords and resolution
	~FrameBuffer();

	// function that is always called back by system and never called directly by programmer
	// programmer triggers framebuffer update by calling FrameBuffer::redraw(), which makes
	//            system call draw
	void draw();

	// function called back when event occurs (mouse, keyboard, etc)
	int handle(int event);
	void KeyboardHandle();
	void mousePushHandle();
	void mouseMoveHandle();

	void Set(unsigned int bgr);
	void Set(int u, int v, unsigned int clr);
	void Set(int u, int v, unsigned int clr, float z);
	void SetGuarded(int u, int v, unsigned int clr, float z);
	void SetZB(float z0);
	void Draw2DSegment(const V3 &p0, const V3 &c0, const V3 &p1, const V3 &c1);
	void Draw3DSegment(PPC* ppc, const V3 &p0, const V3 &c0, const V3 &p1, const V3 &c1);
	void DrawRectangle(const V3 &p0, const V3 &p1, const V3 &c);
	void DrawPPCFrustum(PPC* ppc, PPC* frustum);
	bool Load(char* filename);
	bool Save(char* filename);

	void Draw2DBigPoint(int u, int v, int psize, const V3 &color, float z);
	void Draw3DBigPoint(PPC* ppc, const V3 &p, int psize, const V3 &color);

	bool isHidden(int u, int v, float z);

	void rasterize(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2);
	void rasterizeWithTexture(PPC* ppc, const M33 &camMat, const Vertex &p0, const Vertex &p1, const Vertex &p2, Texture* texture);
};


