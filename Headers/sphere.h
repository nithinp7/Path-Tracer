#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rtObject.h"
#include "scene.h"

class sphere : public rtObject
{
public:
	float mass;
	glm::vec3 center;
	float radius;
	glm::vec3 velocity;
	
	//constructor given  center, radius, and material
	sphere(glm::vec3 c, float r, int m, scene* s);

	void update(float dt);

	void updateUniforms();
private:

};