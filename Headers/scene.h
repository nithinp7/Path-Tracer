#pragma once 

#ifndef SCENE
#define SCENE

#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <iostream>
#include <stb_image.h>

#include "rtObjGroup.h"
#include "light.h"
#include "rtObject.h"
#include "sphere.h"
#include "triangle.h"
#include "material.h"
#include "water.h"
#include "ballPit.h"
#include "plane.h"

class sphere;
class triangle;
class water;
class plane;
class ballPit;

// For parsing purposes
#define MAX_PARSER_TOKEN_LENGTH 100

#define MAX_TRIS 20
#define MAX_SPHERES 50
#define MAX_PLANES 1

#define MAX_TEXTURES 3
#define MAX_MATERIALS 5

#define MAX_LIGHTS 4

//TODO: move uniform-related structs into own file 

typedef struct {
	glm::vec4 difCol;
	glm::vec4 specCol;
	glm::vec4 reflCol;
	glm::vec4 transpCol;

	float shininess;
	float refrIndx;

	int texIndx;

	float _pad;
} _material;

typedef struct {
	glm::vec4 center;
	float radius;
	int matIndx;

	float _pad[2];
} _sphere;

typedef struct {
	glm::vec4 a;
	glm::vec4 ab;
	glm::vec4 ac;
	glm::vec4 n;

	//texture info
	glm::vec2 tex0;
	glm::vec2 tex1;
	glm::vec2 tex2;

	int matIndx;

	float _pad;
} _tri;

typedef struct {
	glm::vec4 p;
	glm::vec4 n;
	glm::vec4 u;
	glm::vec4 v;

	int matIndx;

	float _pad[3];
} _plane;

typedef struct {
	_tri tris[MAX_TRIS];
	_sphere spheres[MAX_SPHERES];
	_plane planes[MAX_PLANES];

	glm::vec4 lights[MAX_LIGHTS];

	_material maters[MAX_MATERIALS];

	glm::vec4 ambLight;
	glm::vec4 bgCol;
	
	glm::vec4 eye;
	glm::vec4 ray00;
	glm::vec4 ray01;
	glm::vec4 ray10;
	glm::vec4 ray11;

	int num_tris;
	int num_spheres;
	int num_planes;
	int num_lights;

	float time;
	//float _pad[3];
} unifs;

class scene
{
public:
	//UNIFORM DETAILS
	int curTriIndx = 0;
	int curSphereIndx = 0;
	int curPlaneIndx = 0;

	unifs ubo_data;

	GLuint ubo;
	GLuint unif_block_indx;

	GLint tex_unifs[MAX_TEXTURES];

	std::vector<GLint> textureIDs;
	std::vector<std::string> textureNames;

	void initUniforms(GLuint compProgID);
	void updateUniforms(GLuint compProgID);

	void updateScene();

	//constructor from a scene text file
	scene(const char* filename, int w, int h);

	//destructor
	~scene();

	rtObjGroup* myObjGroup;

	std::vector<material> myMaterials;
	std::vector<light> myLights;

	glm::vec3 bgColor; //background color
	glm::vec3 ambLight; //ambient light
	glm::vec3 eye; //camera location
	glm::vec3 lookAt; //camera looking at (x, y, z)
	glm::vec3 up; //camera up vector
	float fovy; //camera vertical angle of view
	
	int loadTexture(char const* texname);
private:
	int WIDTH, HEIGHT;

	float time = 0;

	bool isWaterScene = false;
	bool isBallPitScene = false;

	water* water_obj = NULL;
	ballPit* ballPit_obj = NULL;

	void initWaterScene();
	void initBallPitScene();

	//some functions and variables used in parsing
	void parse(const char* filename);
	void eatWhitespace(void);
	int getToken(char token[MAX_PARSER_TOKEN_LENGTH]);
	glm::vec3 readVec3f();
	float readFloat();
	int readInt();
	void parseBackground();
	void parseCamera();
	rtObjGroup* parseGroup();
	sphere* parseSphere();
	void parseMaterials();
	void parseMaterialIndex();
	triangle* parseTriangle();
	void parseLights();
	FILE *file;
	int parse_char; //current character read from file
	int curline;    //current line in the file
};

#endif