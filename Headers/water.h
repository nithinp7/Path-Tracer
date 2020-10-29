#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "rtObject.h"
#include "rtObjGroup.h"
#include "light.h"
#include "material.h"
#include "triangle.h"
#include "sphere.h"
#include "scene.h"

//class sphere;
class rtObject;
class triangle;
/*
class scene;
class light;
class material;
class rtObjGroup;
*/
class water : public rtObject
{
public:
	//constructor given  center, radius, and material
	water(scene* s);

	//see descriptions in rtObject

	void update();

	void updateUniforms();

private:
	
	typedef struct icoord icoord_t;

	int hWidth;
	int hHeight;
	//ping-pong buffer
	float* hMap0;
	float* hMap1;
	//tracks which buffer to overwrite
	bool bufferState = true;

	triangle** mesh;

	glm::vec3 gridToWorld(int i, int j);
	icoord_t worldToGrid(float x, float y);
	int getHmapIndex(int i, int j);
	int getHmapIndex(icoord p);
	int getMeshIndex(int i, int j);
	int getMeshIndex(icoord p);
	bool checkWithin(float x, float y);
};