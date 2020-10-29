#include "triangle.h"

#define EPSILON 1e-4f

//constructor given  center, radius, and material
triangle::triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, float tx0, float tx1, float tx2, float ty0, float ty1, float ty2, int m, scene* s) : rtObject(s)
{
	point0 = p0;
	point1 = p1;
	point2 = p2;

	texX0 = tx0;
	texX1 = tx1;
	texX2 = tx2;
	texY0 = ty0;
	texY1 = ty1;
	texY2 = ty2;

	matIndex = m;
	myScene = s;
}

void triangle::set(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) 
{
	point0 = p0;
	point1 = p1;
	point2 = p2;
}

void triangle::updateUniforms() {
	/*
	tri_uniforms tu = myScene->tris_unifs[myScene->curTriIndx++];

	glm::vec3 ab = point1 - point0;
	glm::vec3 ac = point2 - point0;
	glm::vec3 n = glm::normalize(glm::cross(ab, ac));

	glUniform3f(tu.a, point0.x, point0.y, point0.z);
	glUniform3f(tu.ab, ab.x, ab.y, ab.z);
	glUniform3f(tu.ac, ac.x, ac.y, ac.z);
	glUniform3f(tu.n, n.x, n.y, n.z);

	glUniform2f(tu.tex0, texX0, texY0);
	glUniform2f(tu.tex1, texX1, texY1);
	glUniform2f(tu.tex2, texX2, texY2);
	
	glUniform1i(tu.matIndx, matIndex);
	*/
	glm::vec3 ab = point1 - point0;
	glm::vec3 ac = point2 - point0;
	glm::vec3 n = glm::normalize(glm::cross(ab, ac));

	_tri t = {
		glm::vec4(point0, 1),
		glm::vec4(ab, 1),
		glm::vec4(ac, 1),
		glm::vec4(n, 1),

		glm::vec2(texX0, texY0),
		glm::vec2(texX1, texY1),
		glm::vec2(texX2, texY2),

		matIndex
	};

	myScene->ubo_data.tris[myScene->curTriIndx++] = t;
}