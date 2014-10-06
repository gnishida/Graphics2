#pragma once

#include "V3.h"
#include <vector>

class Texture {
private:
	std::vector<int> widths;
	std::vector<int> heights;
	std::vector<unsigned int*> images;

	int mipmap_id1;
	int mipmap_id2;
	float mipmap_s;

public:
	Texture(const char* filename);
	~Texture();

	V3 GetColor(float s, float t);
	void SetMipMap(int width, int height, float ds, float dt);

private:
	V3 GetColor(unsigned int* image, int width, int height, float u, float v);
	void CreateMipMap(int width, int height);
};

