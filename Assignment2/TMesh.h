#pragma once

#include "V3.h"
#include "PPC.h"
#include "Texture.h"

class FrameBuffer;

typedef struct {
	V3 v;
	V3 c;
	V3 n;
	float t[2];
} Vertex;

class TMesh {
protected:
	Vertex* verts;
	int vertsN;

	unsigned int* tris;
	int trisN;

	Texture* texture;
	/*
	unsigned int* texture;
	int t_w;
	int t_h;
	*/

public:
	TMesh();
	~TMesh();

	void Load(char *filename);
	void ComputeAABB(AABB &aabb);
	void Translate(const V3 &v);
	void Scale(float t);
	void Scale(const V3 &centroid, const V3 &size);
	void RenderWireframe(FrameBuffer *fb, PPC *ppc);
	void Render(FrameBuffer *fb, PPC *ppc);

	void Clear();
	void RotateAbout(const V3 &axis, float angle);
	void RotateAbout(const V3 &axis, float angle, const V3 &orig);
	V3 GetCentroid();

	bool isInside2D(const V3 &p0, const V3 &p1, const V3 &p2, const V3 p) const;
	bool SetTexture(const char* filename);
	//V3 interpolate(const V3 &c0, const V3 &c1, const V3 &c2, float s, float t) const;
};

