#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rtObject.h"
#include "scene.h"

class triangle : public rtObject
{
public:
	//constructor given points in 3d space and corresponding
	//locations on the texture surface
	triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, float tx0, float tx1, float tx2, float ty0, float ty1, float ty2, int m, scene* s);

	void set(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2);

	void updateUniforms();

private:
	//the points in 3d space that define the triangle
	glm::vec3 point0, point1, point2;
	//the x-y locations on the texture image corresponding to each vertex
	float texX0, texX1, texX2, texY0, texY1, texY2;
};
