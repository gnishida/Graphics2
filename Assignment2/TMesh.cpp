#include "TMesh.h"
#include "FrameBuffer.h"
#include <libtiff/tiffio.h>
#include <fstream>
#include <iostream>
#include <limits>

using namespace std;

TMesh::TMesh() {
	verts = NULL;
	vertsN = 0;
	tris = NULL;
	trisN = 0;

	texture = NULL;
}

TMesh::~TMesh() {
	if (texture != NULL) {
		delete texture;
	}
}

/**
 * Load a mesh from bin file.
 */
void TMesh::Load(char* filename) {
	Clear();

	ifstream ifs(filename, ios::binary);
	if (ifs.fail()) {
		cerr << "INFO: cannot open file: " << filename << endl;
		return;
	}

	ifs.read((char*)&vertsN, sizeof(int));
	char v_yn, c_yn, n_yn, t_yn;
	ifs.read(&v_yn, 1); // always xyz
	if (v_yn != 'y') {
		cerr << "INTERNAL ERROR: there should always be vertex xyz data" << endl;
		return;
	}

	verts = new Vertex[vertsN];

	ifs.read(&c_yn, 1); // cols 3 floats
	ifs.read(&n_yn, 1); // normals 3 floats
	ifs.read(&t_yn, 1); // texture coordinates 2 floats

	for (int i = 0; i < vertsN; i++) {
		ifs.read((char*)&verts[i].v[0], 3 * sizeof(float));
	}

	if (c_yn == 'y') {
		for (int i = 0; i < vertsN; i++) {
			ifs.read((char*)&verts[i].c[0], 3 * sizeof(float));
		}
	}

	if (n_yn == 'y') {
		for (int i = 0; i < vertsN; i++) {
			ifs.read((char*)&verts[i].n[0], 3 * sizeof(float));
		}
	}

	if (t_yn == 'y') {
		for (int i = 0; i < vertsN; i++) {
			ifs.read((char*)&verts[i].t[0], 2 * sizeof(float));
		}
	}

	ifs.read((char*)&trisN, sizeof(int));
	tris = new unsigned int[trisN*3];
	ifs.read((char*)tris, trisN*3*sizeof(unsigned int)); // read tiangles

	ifs.close();

	//cerr << "INFO: loaded " << vertsN << " verts, " << trisN << " tris from " << endl << "      " << filename << endl;
	//cerr << "      xyz " << ((cols) ? "rgb " : "") << ((norms) ? "nxnynz " : "") << ((tcs) ? "tcstct " : "") << endl;
}

/**
 * Compute 3D axis aligned bouding box.
 *
 * @param p0	the corner with the minimum coordinates
 * @param p1	the corner with the maximum coordinates
 */
void TMesh::ComputeAABB(AABB &aabb) {
	for (int i = 0; i < vertsN; i++) {
		aabb.AddPoint(verts[i].v);
	}
}

/**
 * Translate all the vertices by the specified vector.
 *
 * @param v		the specified vector
 */
void TMesh::Translate(const V3 &v) {
	for (int i = 0; i < vertsN; i++) {
		verts[i].v += v;
	}
}

/**
 * Scale all the vertices by the specified factor.
 *
 * @param t		the specified scaling factor
 */
void TMesh::Scale(float t) {
	for (int i = 0; i < vertsN; i++) {
		verts[i].v *= t;
	}
}

/**
 * Scale this mesh by placing the centroid at givin position and scaling to given AABB size.
 *
 * @param centroid		the given position for the new centroid
 * @param size			the given AABB size
 */
void TMesh::Scale(const V3 &centroid, const V3 &size) {
	AABB aabb;
	ComputeAABB(aabb);

	V3 c = GetCentroid();
	
	V3 scale(size.x() / aabb.Size().x(), size.y() / aabb.Size().y(), size.z() / aabb.Size().z());

	for (int i = 0; i < vertsN; i++) {
		verts[i].v[0] = (verts[i].v.x() - c.x()) * scale.x() + centroid.x();
		verts[i].v[1] = (verts[i].v.y() - c.y()) * scale.y() + centroid.y();
		verts[i].v[2] = (verts[i].v.z() - c.z()) * scale.z() + centroid.z();
	}
}

void TMesh::RenderWireframe(FrameBuffer *fb, PPC *ppc) {
	for (int i = 0; i < trisN; i++) {
		fb->Draw3DSegment(ppc, verts[tris[i * 3]].v, verts[tris[i * 3]].c, verts[tris[i * 3 + 1]].v, verts[tris[i * 3 + 1]].c);
		fb->Draw3DSegment(ppc, verts[tris[i * 3 + 1]].v, verts[tris[i * 3 + 1]].c, verts[tris[i * 3 + 2]].v, verts[tris[i * 3 + 2]].c);
		fb->Draw3DSegment(ppc, verts[tris[i * 3 + 2]].v, verts[tris[i * 3 + 2]].c, verts[tris[i * 3]].v, verts[tris[i * 3]].c);
	}
}

void TMesh::Render(FrameBuffer *fb, PPC *ppc) {
	M33 camMat;
	camMat.SetColumn(0, ppc->a);
	camMat.SetColumn(1, ppc->b);
	camMat.SetColumn(2, ppc->c);

	if (texture != NULL) {
		for (int i = 0; i < trisN; i++) {
			fb->rasterizeWithTexture(ppc, camMat, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]], texture);
		}
	} else {
		for (int i = 0; i < trisN; i++) {
			fb->rasterize(ppc, camMat, verts[tris[i * 3]], verts[tris[i * 3 + 1]], verts[tris[i * 3 + 2]]);
		}
	}
}

/**
 * Clear the allocated memory for vertices.
 */
void TMesh::Clear() {
	if (verts != NULL) {
		delete [] verts;
	}
	verts = NULL;
	
	vertsN = 0;

	if (tris != NULL) {
		delete [] tris;
	}
	trisN = 0;
}

/**
 * Rotate this mesh around the specified axis passing through the centroid by the specified angle.
 *
 * @param axis		the specified axis
 * @param angle		the specified angle
 */
void TMesh::RotateAbout(const V3 &axis, float angle) {
	RotateAbout(axis, angle, GetCentroid());
}

/**
 * Rotate this mesh around the specified axis passing through the specified origin by the specified angle.
 *
 * @param axis		the specified axis
 * @param angle		the specified angle
 * @param orig		the specified origin
 */
void TMesh::RotateAbout(const V3 &axis, float angle, const V3 &orig) {
	for (int i = 0; i < vertsN; i++) {
		verts[i].v = verts[i].v.RotateAbout(axis, angle, orig);
	}
}

/**
 * Return the center of axis aligned bounding box of vertices.
 *
 * @return		the center of axis aligned bounding box
 */
V3 TMesh::GetCentroid() {
	AABB aabb;
	ComputeAABB(aabb);

	return (aabb.maxCorner() + aabb.minCorner()) / 2.0f;
}

bool TMesh::isInside2D(const V3 &p0, const V3 &p1, const V3 &p2, const V3 p) const {
	float den = ((p1 - p0) ^ (p2 - p0)).z();
	float s = ((p - p0) ^ (p2 - p0)).z() / den;
	float t = ((p - p0) ^ (p1 - p0)).z() / den;

	if (s < 0 || s > 1) return false;
	if (t < 0 || t > 1) return false;
	if (s + t > 1) return false;
	return true;
}

bool TMesh::SetTexture(const char* filename) {
	texture = new Texture(filename);
	return true;
}

/*V3 TMesh::interpolate(const V3 &c0, const V3 &c1, const V3 &c2, float s, float t) const {
	return c0 * (1 - s - t) + c1 * s + c2 * t;
}*/