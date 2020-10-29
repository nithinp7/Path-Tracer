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
#include "plane.h"

class rtObject;
class plane;
class sphere;

class ballPit
{
public:
	ballPit(scene* s);

	void update();

private:
	int ballCount;
	sphere** balls;

	plane* floorPlane;
};