#include "water.h"

#define EPSILON 1e-4f
#define hMap(X,Y) (bufferState ? hMap1[(Y)*hWidth+(X)] : hMap0[(Y)*hWidth+(X)])

//NOTE: this entire scene is hard-coded for now, but it could easily be integrated with
//the existing generalized parser system

typedef struct icoord {
	int i;
	int j;
} icoord_t;

water::water(scene* s) : rtObject(s)
{
	myScene = s;

	s->bgColor = glm::vec3(0.05, 0.05, 0.05);
	s->ambLight = glm::vec3(0.2, 0.2, 0.2);

	s->eye = glm::vec3(0, 3, -7);
	s->lookAt = glm::vec3(0, 0, 0);
	s->up = glm::vec3(0, 1, 0);
	s->fovy = 45;

	light l1;
	l1.position = glm::vec3(11, 20, 10);
	l1.color = glm::vec3(1, 1, 1);
	s->myLights.push_back(l1);

	light l2;
	l2.position = glm::vec3(30, 10, 0);
	l2.color = glm::vec3(0.3, 0.3, 0.3);
	s->myLights.push_back(l2);

	material m0;
	m0.diffuseCol = glm::vec3(0.1f, 0.3f, 0.8f);
	m0.specularCol = glm::vec3(0.8f, 0.8f, 0.8f);
	m0.reflectiveCol = glm::vec3(0.7f, 0.7f, 0.7f);
	m0.shininess = 50.0f;
	m0.transparentCol = glm::vec3(0.0f, 0.0f, 0.0f);
	m0.refractionIndex = 0.6f;
	m0.textureIndex = -1;

	s->myMaterials.push_back(m0);

	material m1;
	m1.diffuseCol = glm::vec3(0.8f, 0.3f, 0.1f);
	m1.specularCol = glm::vec3(0.8f, 0.8f, 0.8f);
	m1.reflectiveCol = glm::vec3(0.5f, 0.5f, 0.5f);
	m1.shininess = 50.0f;
	m1.transparentCol = glm::vec3(0.0f, 0.0f, 0.0f);
	m1.refractionIndex = 0.6f;
	m1.textureIndex = -1;

	s->myMaterials.push_back(m1);

	sphere* sp = new sphere(glm::vec3(0, 2, 8), 3, 1, s);

	s->myObjGroup = new rtObjGroup();
	s->myObjGroup->addObj(sp);

	//s->myObjGroup->addObj(this);

	hWidth = hHeight = 10;
	hMap0 = (float*)malloc(hWidth * hHeight * sizeof(float));
	hMap1 = (float*)malloc(hWidth * hHeight * sizeof(float));
	mesh = (triangle**)malloc(2 * (hWidth - 1) * (hHeight - 1) * sizeof(triangle*));

	//GENERATE PERLIN NOISE
	/*
	float* noise = (float*) malloc(hWidth*hHeight * sizeof(float));
	for (int i = 0; i < hWidth; i++)
		for (int j = 0; j < hHeight; j++)
			noise[j*hWidth + i] = (float)rand() / (float)RAND_MAX;
	*/

	//
	for (int i = 0; i < hWidth; i++)
		for (int j = 0; j < hHeight; j++) {
			float r = 0.4f * (2.0f * (float)rand() / (float)RAND_MAX - 1.0f);
			if (i < 5 && j < 5) {
				hMap0[j * hWidth + i] = 0.5f;//noise(i, j);
				hMap1[j * hWidth + i] = 0.5f;
			}
			else {
				hMap0[j * hWidth + i] = 0;
				hMap1[j * hWidth + i] = 0;
			}
			hMap0[j * hWidth + i] = r;
			hMap1[j * hWidth + i] = r;
		}

	for (int i = 0; i < hWidth - 1; i++)
		for (int j = 0; j < hHeight - 1; j++) {
			glm::vec3 p0, p1, p2, p3;
			p0 = gridToWorld(i, j);
			p1 = gridToWorld(i + 1, j);
			p2 = gridToWorld(i, j + 1);
			p3 = gridToWorld(i + 1, j + 1);

			triangle *t0 = new triangle(p0, p1, p3, 0, 1, 1, 0, 0, 1, 0, s);
			triangle *t1 = new triangle(p0, p3, p2, 0, 1, 0, 0, 1, 1, 0, s);

			s->myObjGroup->addObj(t0);
			s->myObjGroup->addObj(t1);

			mesh[2 * (j * (hWidth - 1) + i)] = t0;
			mesh[2 * (j * (hWidth - 1) + i) + 1] = t1;
		}

	for (int i = 0; i < 20; i++) update();
}

void water::update()
{
	//physics nonsense
	float waveSpeed = 5.0f;
	float damping = 0.97f;//0.97f;
	float dt = 1.0f / 30.0f;
	//...update grid
	float* h0;
	float* h1;
	if (bufferState) {
		h0 = hMap0;
		h1 = hMap1;
	}
	else {
		h0 = hMap1;
		h1 = hMap0;
	}
	for (int i = 0; i < hWidth; i++)
		for (int j = 0; j < hHeight; j++) {
			int p = getHmapIndex(i, j);
			int pr = getHmapIndex(i + 1, j);
			int pl = getHmapIndex(i - 1, j);
			int pu = getHmapIndex(i, j + 1);
			int pd = getHmapIndex(i, j - 1);
			h0[p] =
				waveSpeed * waveSpeed * dt * dt
				* (h1[pr] + h1[pl] + h1[pu] + h1[pd] - 4 * h1[p])
				+ damping * (-h0[p] + 2 * h1[p]);
		}
	//

	for (int i = 0; i < hWidth - 1; i++)
		for (int j = 0; j < hHeight - 1; j++) {
			glm::vec3 p0, p1, p2, p3;
			p0 = gridToWorld(i, j);
			p1 = gridToWorld(i + 1, j);
			p2 = gridToWorld(i, j + 1);
			p3 = gridToWorld(i + 1, j + 1);

			triangle* t0 = mesh[2 * (j * (hWidth - 1) + i)];
			triangle* t1 = mesh[2 * (j * (hWidth - 1) + i) + 1];

			t0->set(p0, p1, p3);
			t1->set(p0, p3, p2);
		}

	bufferState = !bufferState;
}

void water::updateUniforms() {

}

glm::vec3 water::gridToWorld(int i, int j) 
{
	return glm::vec3(-5.0f + 10.0f * i / hWidth, hMap(i, j) - 3.0f, 10.0f * j / hHeight);
}

icoord_t water::worldToGrid(float x, float y)
{
	return { (int)floor((x + 5.0f) / 10.0f * hWidth), (int)floor(y / 10.0f * hHeight) };
}

int water::getHmapIndex(int i, int j)
{
	if (i >= hWidth)
		i = 2 * hWidth - i - 1;
	if (j >= hHeight)
		j = 2 * hHeight - j - 1;
	if (i < 0)
		i *= -1;
	if (j < 0)
		j *= -1;
	return j * hWidth + i;
}

int water::getHmapIndex(icoord_t p)
{
	return getHmapIndex(p.i, p.j);
}

int water::getMeshIndex(int i, int j)
{
	if (i >= hWidth-1)
		i = 2 * (hWidth-1) - i - 1;
	if (j >= hHeight-1)
		j = 2 * (hHeight-1) - j - 1;
	if (i < 0)
		i *= -1;
	if (j < 0)
		j *= -1;
	return j * (hWidth-1) + i;
}

int water::getMeshIndex(icoord_t p)
{
	return getMeshIndex(p.i, p.j);
}

bool water::checkWithin(float x, float y)
{
	return x <= 5.0f && x >= -5.0f && y <= 10.0f && y >= 0.0f;
}
