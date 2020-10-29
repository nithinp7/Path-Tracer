#include "sphere.h"

# define M_PI           3.14159265358979323846f  /* pi */

//constructor given  center, radius, and material
sphere::sphere(glm::vec3 c, float r, int m, scene* s) : rtObject(s)
{
	center = c;
	velocity = glm::vec3(0, 0, 0);
	radius = r;
	matIndex = m;
	myScene = s;
	mass = 4.0f * M_PI * r * r * r / 3.0f;
}

void sphere::update(float dt) 
{
	velocity += dt * glm::vec3(0, -0.9, 0);
	center += dt * velocity;
}

void sphere::updateUniforms() {
	/*
	sphere_uniforms su = myScene->spheres_unifs[myScene->curSphereIndx++];

	glUniform3f(su.center, center.x, center.y, center.z);
	glUniform1f(su.radius, radius);
	glUniform1i(su.matIndx, matIndex);
	*/

	_sphere s = {
		glm::vec4(center, 1),
		radius,

		matIndex
	};

	myScene->ubo_data.spheres[myScene->curSphereIndx++] = s;
}