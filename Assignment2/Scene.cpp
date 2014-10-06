#include "Scene.h"
#include "V3.h"
#include "Box.h"
#include "Triangle.h"
#include "Quad.h"
#include "Sphere.h"
#include "Light.h"
#include <time.h>
#include <float.h>
#include <iostream>
#include <fstream>

using namespace std;

Scene *scene;

int Scene::rasterization_mode = SCREEN_SPACE_RASTERIZATION;
int Scene::shading_mode = NO_SHADING;
Light* Scene::light = new Light(V3(0.0f, 0.0f, -1.0f), Light::TYPE_DIRECTIONAL_LIGHT, 0.4f, 0.6f, 40.0f);

Scene::Scene() {
	// create user interface
	gui = new GUI();
	gui->show();

	// create SW framebuffer
	int u0 = 20;
	int v0 = 50;
	int sci = 2;
	int w = sci*240;//640;
	int h = sci*180;//360;
	fb = new FrameBuffer(u0, v0, w, h);
	fb->label("SW Framebuffer");
	fb->show();
  
	// position UI window
	gui->uiw->position(fb->w+u0 + 2*20, v0);

	tmsN = 9;
	tms = new TMesh*[tmsN];
	tms[0] = new TMesh();
	tms[0]->Load("geometry/teapot1K.bin");
	tms[0]->Translate(V3(-50.0f, 0.0f, 0.0f) - tms[0]->GetCentroid());
	tms[1] = new TMesh();
	tms[1]->Load("geometry/teapot1K.bin");
	tms[1]->Translate(V3(50.0f, 0.0f, 0.0f) - tms[1]->GetCentroid());
	tms[2] = new TMesh();
	tms[2]->Load("geometry/teapot1K.bin");
	tms[2]->Translate(V3(200.0f, 0.0f, 0.0f) - tms[2]->GetCentroid());

	tms[3] = new Quad(60, 60, V3(0.0f, 0.0f, 0.0f));
	tms[3]->SetTexture("texture/mycamera.tif");
	tms[3]->Translate(V3(300.0f, 0.0f, 0.0f) - tms[3]->GetCentroid());
	tms[4] = new Quad(60, 60, V3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 3.0f, 4.14f);
	tms[4]->SetTexture("texture/tile.tif");
	tms[4]->Translate(V3(370.0f, 0.0f, 0.0f) - tms[4]->GetCentroid());
	tms[5] = new Quad(60, 60, V3(0.0f, 0.0f, 0.0f));
	tms[5]->SetTexture("texture/web.tif");
	tms[5]->Translate(V3(440.0f, 0.0f, 0.0f) - tms[5]->GetCentroid());
	tms[6] = new Quad(60, 60, V3(0.0f, 0.0f, 0.0f));
	tms[6]->SetTexture("texture/complex_lighting.tif");
	tms[6]->Translate(V3(510.0f, 0.0f, 0.0f) - tms[6]->GetCentroid());
	tms[7] = new Quad(60, 60, V3(0.0f, 0.0f, 0.0f));
	tms[7]->SetTexture("texture/reflection.tif");
	tms[7]->Translate(V3(580.0f, 0.0f, 0.0f) - tms[7]->GetCentroid());
	tms[8] = new Sphere(120, V3(0, 0, 1.0f), 20, 40);
	tms[8]->Translate(V3(520.0f, 0.0f, -200.0f));
	tms[8]->SetTexture("texture/earth.tif");


	// create three cameras
	ppcN = 3;
	ppc = new PPC*[ppcN];
	float hfov = 60.0f;
	ppc[0] = new PPC(hfov, fb->w, fb->h);
	ppc[0]->LookAt(V3(0.0f, 0.0f, 0.0f), V3(0.0f, 0.0f, -1.0f), V3(0.0f, 1.0f, 0.0f), 200.0f);
	ppc[1] = new PPC(hfov, fb->w, fb->h);
	ppc[1]->LookAt(tms[2]->GetCentroid(), V3(0.0f, 0.0f, -1.0f), V3(0.0f, 1.0f, 0.0f), 200.0f);
	ppc[2] = new PPC(hfov, fb->w, fb->h);
	ppc[2]->LookAt(tms[6]->GetCentroid(), V3(0.0f, 0.0f, -1.0f), V3(0.0f, 1.0f, 0.0f), 200.0f);
	currentPPC = ppc[0];
	
	rasterization_mode = MODEL_SPACE_RASTERIZATION;
	//shading_mode = GOURAUD_SHADING;
	shading_mode = PHONG_SHADING;

	Render();

	//SaveTIFFs();
}

// function linked to the DBG GUI button for testing new features
void Scene::DBG() {

}

/**
 * This function is called when "Demo" button is clicked.
 */
void Scene::Demo() {
	currentPPC = ppc[0];

	for (int i = 0; i < 150; i++) {
		light->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.6f, V3(0.0f, 0.0f, 0.0f));
		Render();
		Fl::wait();
	}

	for (int i = 0; i < 150; i++) {
		PPC p = ppc[0]->Interpolate(*ppc[1], (float)i / 150.0f);
		tms[8]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.2f, tms[8]->GetCentroid());

		currentPPC = &p;
		Render();
		Fl::wait();
	}

	delete light;
	light = new Light(V3(240.0f, 0.0f, 0.0f), Light::TYPE_POINT_LIGHT, 0.4f, 0.6f, 40.0f);

	currentPPC = ppc[1];
	for (int i = 0; i < 150; i++) {
		light->RotateAbout(V3(0.0f, 1.0f, 0.0f), -0.6f, tms[2]->GetCentroid());
		tms[8]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.2f, tms[8]->GetCentroid());

		Render();
		Fl::wait();
	}

	for (int i = 0; i < 150; i++) {
		PPC p = ppc[1]->Interpolate(*ppc[2], (float)i / 150.0f);
		for (int j = 3; j <= 7; j++) {
			if (i < 15 || (int)((i - 15) / 30) % 2 == 1) {
				tms[j]->RotateAbout(V3(0.0f, 1.0f, 0.0f), -2.0f, tms[j]->GetCentroid());
			} else {
				tms[j]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 2.0f, tms[j]->GetCentroid());
			}
		}
		tms[8]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.2f, tms[8]->GetCentroid());

		currentPPC = &p;
		Render();
		Fl::wait();
	}

	currentPPC = ppc[2];
}

/**
 * This function is called when "SaveTIFFs" button is clicked.
 * The scene is animated with total 600 frames, which are corresponding to 20 seconds of animation,
 * and all the frames are stored in "captured" folder.
 */
void Scene::SaveTIFFs() {
	int count = 0;
	char filename[32];

	currentPPC = ppc[0];

	for (int i = 0; i < 150; i++) {
		light->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.6f, V3(0.0f, 0.0f, 0.0f));
		Render();
		sprintf(filename, "captured\\scene%03d.tif", count++);
		fb->Save(filename);
		Fl::wait();
	}

	for (int i = 0; i < 150; i++) {
		PPC p = ppc[0]->Interpolate(*ppc[1], (float)i / 150.0f);
		tms[8]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.2f, tms[8]->GetCentroid());

		currentPPC = &p;
		Render();
		sprintf(filename, "captured\\scene%03d.tif", count++);
		fb->Save(filename);
		Fl::wait();
	}

	delete light;
	light = new Light(V3(240.0f, 0.0f, 0.0f), Light::TYPE_POINT_LIGHT, 0.4f, 0.6f, 40.0f);

	currentPPC = ppc[1];
	for (int i = 0; i < 150; i++) {
		light->RotateAbout(V3(0.0f, 1.0f, 0.0f), -0.6f, tms[2]->GetCentroid());
		tms[8]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.2f, tms[8]->GetCentroid());

		Render();
		sprintf(filename, "captured\\scene%03d.tif", count++);
		fb->Save(filename);
		Fl::wait();
	}

	for (int i = 0; i < 150; i++) {
		PPC p = ppc[1]->Interpolate(*ppc[2], (float)i / 150.0f);
		for (int j = 3; j <= 7; j++) {
			if (i < 15 || (int)((i - 15) / 30) % 2 == 1) {
				tms[j]->RotateAbout(V3(0.0f, 1.0f, 0.0f), -2.0f, tms[j]->GetCentroid());
			} else {
				tms[j]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 2.0f, tms[j]->GetCentroid());
			}
		}
		tms[8]->RotateAbout(V3(0.0f, 1.0f, 0.0f), 0.2f, tms[8]->GetCentroid());

		currentPPC = &p;
		Render();
		sprintf(filename, "captured\\scene%03d.tif", count++);
		fb->Save(filename);
		Fl::wait();
	}

	currentPPC = ppc[2];
}

/**
 * Render all the models.
 */
void Scene::Render() {
	fb->SetZB(0.0f);
	fb->Set(BLACK);

	for (int i = 0; i < tmsN; i++) {
		if (i == 0) {
			shading_mode = GOURAUD_SHADING;
		} else {
			shading_mode = PHONG_SHADING;
		}
		tms[i]->Render(fb, currentPPC);
	}

	/*
	for (int i = 0; i < ppcN; i++) {
		if (ppc[i] == currentPPC) continue;
		ppc[i]->DrawPPCFrustum(currentPPC, fb, 0.1f);
	}
	*/

	fb->redraw();
}