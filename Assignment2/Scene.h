#pragma once

#include "gui.h"
#include "FrameBuffer.h"
#include "V3.h"
#include "M33.h"
#include "PPC.h"
#include "TMesh.h"
#include "Light.h"
#include <vector>
#include <iostream>

#define SCREEN_SPACE_RASTERIZATION	0
#define MODEL_SPACE_RASTERIZATION	1

#define NO_SHADING					0
#define GOURAUD_SHADING				1
#define PHONG_SHADING				2

using namespace std;

class Scene {
public:
	/** Software frame buffer */
	FrameBuffer *fb;

	/** Planar pinhole camera */
	PPC** ppc;
	int ppcN;
	PPC* currentPPC;

	/** Graphical user interface */
	GUI *gui;

	/** Triangle mesh object */
	TMesh** tms;
	//vector<TMesh*> meshes;

	/** The number of triangle meshes */
	int tmsN;

	static Light* light;
	static int rasterization_mode;
	static int shading_mode;

public:
	Scene();
	void DBG();
	void Demo();
	void SaveTIFFs();
	void Render();
};

extern Scene *scene;

