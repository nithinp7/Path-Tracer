#include "rtObjGroup.h"

//default constructor
rtObjGroup::rtObjGroup()
{}

//add an object to the vector
void rtObjGroup::addObj(rtObject *o)
{
	myObjects.push_back(o);
}

//accessor for objects in the vector
rtObject* rtObjGroup::getObj(int index)
{
	return myObjects.at(index);
}

void rtObjGroup::updateUniforms() {
	for (int i = 0; i < myObjects.size(); i++) 
		myObjects.at(i)->updateUniforms();
}