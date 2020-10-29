#include "plane.h"

//constructor given  center, radius, and material
plane::plane(glm::vec3 p, glm::vec3 n, glm::vec3 u_, glm::vec3 v_, int m, scene* s) : rtObject(s)
{
	point = p;
	normal = n;
	u = u_;
	v = v_;
	matIndex = m;
	myScene = s;
}

void plane::updateUniforms() {
	/*
	plane_uniforms pu = myScene->planes_unifs[myScene->curPlaneIndx++];

	glUniform3f(pu.p, point.x, point.y, point.z);
	glUniform3f(pu.n, normal.x, normal.y, normal.z);
	glUniform3f(pu.u, u.x, u.y, u.z);
	glUniform3f(pu.v, v.x, v.y, v.z);

	glUniform1i(pu.matIndx, matIndex);
	*/
	_plane p = {
		glm::vec4(point, 1),
		glm::vec4(normal, 1),
		glm::vec4(u, 1),
		glm::vec4(v, 1),

		matIndex
	};

	myScene->ubo_data.planes[myScene->curPlaneIndx++] = p;
}