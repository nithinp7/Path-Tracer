#include "ballPit.h"
 
#define EPSILON 1e-4f

ballPit::ballPit(scene* s) 
{
	s->bgColor = glm::vec3(0.05, 0.05, 0.05);
	s->ambLight = glm::vec3(0.2, 0.2, 0.2);

	s->eye = glm::vec3(0, 3, -50);
	s->lookAt = glm::vec3(0, 0, 0);
	s->up = glm::vec3(0, 1, 0);
	s->fovy = 45;

	light l1;
	l1.position = glm::vec3(11, 20, 30);
	l1.color = glm::vec3(1, 1, 1);
	s->myLights.push_back(l1);

	light l2;
	l2.position = glm::vec3(30, 10, 20);
	l2.color = glm::vec3(0.3, 0.3, 0.3);
	s->myLights.push_back(l2);

	material m0;
	m0.diffuseCol = glm::vec3(0.1f, 0.1f, 0.1f);//glm::vec3(0.1f, 0.3f, 0.8f);
	m0.specularCol = glm::vec3(0.8f, 0.8f, 0.8f);
	m0.reflectiveCol = glm::vec3(0.4f, 0.4f, 0.4f);
	m0.shininess = 50.0f;
	m0.transparentCol = glm::vec3(0.6f, 0.6f, 0.6f);
	m0.refractionIndex = 0.85f;
	m0.textureIndex = -1;

	s->myMaterials.push_back(m0);

	material m1;
	m1.diffuseCol = glm::vec3(0.5f, 0.1f, 0.1f);
	m1.specularCol = glm::vec3(0.8f, 0.8f, 0.8f);
	m1.reflectiveCol = glm::vec3(0.1f, 0.1f, 0.1f); //glm::vec3(0.3f, 0.3f, 0.3f);
	m1.shininess = 50.0f;
	m1.transparentCol = glm::vec3(0.9f, 0.9f, 0.9f);
	m1.refractionIndex = 0.85f;
	m1.textureIndex = -1;

	s->myMaterials.push_back(m1);

	material m2;
	m2.diffuseCol = glm::vec3(0.9f, 0.6f, 0.1f);
	m2.specularCol = glm::vec3(0.8f, 0.8f, 0.8f);
	m2.reflectiveCol = glm::vec3(0.0f, 0.0f, 0.0f); //glm::vec3(0.3f, 0.3f, 0.3f);
	m2.shininess = 50.0f;
	m2.transparentCol = glm::vec3(1.0f, 1.0f, 1.0f);
	m2.refractionIndex = 0.9f;
	m2.textureIndex = -1;

	s->myMaterials.push_back(m2);

	material m3;
	m3.diffuseCol = glm::vec3(0.3f, 0.3f, 0.3f);
	m3.specularCol = glm::vec3(0.8f, 0.8f, 0.8f);
	m3.reflectiveCol = glm::vec3(0.0f, 0.0f, 0.0f);//glm::vec3(1.0f, 1.0f, 1.0f);
	m3.shininess = 50.0f;
	m3.transparentCol = glm::vec3(0.0f, 0.0f, 0.0f);
	m3.refractionIndex = 0.6f;
	m3.textureIndex = -1;//s->loadTexture("../RaytracerGPU/Media/textures/marble.jpg");

	s->myMaterials.push_back(m3);

	s->myObjGroup = new rtObjGroup();

	floorPlane = new plane(glm::vec3(0, -20, 0), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), 3, s);
	s->myObjGroup->addObj(floorPlane);

	ballCount = 50;
	balls = (sphere**)malloc(ballCount * sizeof(sphere*));

	for (int i = 0; i < ballCount; i++) {
		sphere* sp = new sphere(glm::vec3(-10 + 1 * i, -5 + 4 * i, 30 + fmod(53 * i, 30)), 0.5f * ((73 * i) % 5 + 3), i % 3, s);
		balls[i] = sp;
		s->myObjGroup->addObj(sp);
	}

	for (int i = 0; i < 120; i++) update();
}

void ballPit::update()
{
	int stepsPerFrame = 2;
	for (int step = 0; step < stepsPerFrame; step++) {
		float elasticity = 0.4f;

		//first move all balls by their velocity
		for (int i = 0; i < ballCount; i++) {
			balls[i]->update(0.1f);
		}

		for (int colStep = 0; colStep < 1; colStep++) {
			//now check for collisions between balls
			for (int i = 0; i < ballCount; i++) {
				sphere* a = balls[i];
				for (int j = i + 1; j < ballCount; j++) {
					sphere* b = balls[j];
					glm::vec3 dif = b->center - a->center;
					glm::vec3 vdif = b->velocity - a->velocity;
					float dist = glm::length(dif);
					glm::vec3 n = glm::normalize(dif);
					//check intersecting
					if (dist < a->radius + b->radius) {
						if (colStep == 0 && glm::dot(vdif, n) < 0) {
							glm::vec3 proj = elasticity * glm::dot(vdif, dif) / dist / dist * dif;
							glm::vec3 vfa = a->velocity + 2 * b->mass / (a->mass + b->mass) * proj;
							glm::vec3 vfb = b->velocity - 2 * a->mass / (a->mass + b->mass) * proj;

							a->velocity = vfa;
							b->velocity = vfb;

							glm::vec3 mtv = (fabs(dist - a->radius - b->radius)*0.02f) * n;
							a->center -= 0.5f * mtv;
							b->center += 0.5f * mtv;
						}
					}
				}
			}

			//now check for ball-plane collisions
			for (int i = 0; i < ballCount; i++) {
				sphere* s = balls[i];
				float dist = glm::dot(floorPlane->normal, s->center - floorPlane->point);
				if (dist < s->radius) {
					if (colStep == 0 && glm::dot(s->velocity, floorPlane->normal) < 0) {
						//decompose velocities into normal and tangential compononents
						glm::vec3 v_proj_n = glm::dot(floorPlane->normal, s->velocity) * floorPlane->normal;
						glm::vec3 v_perp_n = s->velocity - v_proj_n;

						//reflect normal component of velocity
						s->velocity = v_perp_n - elasticity * v_proj_n;
						s->center += (fabs(dist - s->radius)*0.01f) * floorPlane->normal;
					}
				}
			}
		}
	}
}