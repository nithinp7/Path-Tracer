#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


class scene;

//object is a virtual class: it is never used, except as a subclass
class rtObject
{
public:
	//virtual class so constructor is unusable
	rtObject(scene* s);

	//accessor for material index
	virtual int getMatIndex();

	//update the compute shader with uniforms
	virtual void updateUniforms() = 0;

protected:
	scene* myScene;
	int matIndex;

};