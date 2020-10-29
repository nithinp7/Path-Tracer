#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rtObject.h"
#include "scene.h"

class plane : public rtObject
{
public:
	glm::vec3 point;
	glm::vec3 normal;
	//these must be on the plane
	glm::vec3 u;
	glm::vec3 v;

	//constructor given  center, radius, and material
	plane(glm::vec3 p, glm::vec3 n, glm::vec3 u_, glm::vec3 v_, int m, scene* s);

	void updateUniforms();
private:

};