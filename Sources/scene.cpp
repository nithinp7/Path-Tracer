#include "scene.h"

#define EPSILON 1e-3f
#define hMap(X,Y) (bufferState ? hMap1[(Y)*hWidth+(X)] : hMap0[(Y)*hWidth+(X)])

scene::scene(const char* filename, int w, int h)
{
	WIDTH = w;
	HEIGHT = h;

	parse(filename);
	std::cout << std::endl << "background: " << glm::to_string(bgColor);
	std::cout << std::endl << "ambient: " << glm::to_string(ambLight);
	std::cout << std::endl << "eye: " << glm::to_string(eye);
	std::cout << std::endl << "lookAt: " << glm::to_string(lookAt);
	std::cout << std::endl << "up: " << glm::to_string(up);
	std::cout << std::endl << "fovy: " << fovy;
	std::cout << std::endl;
}

void scene::initUniforms(GLuint compProgID) {
	ubo = 0;

	//ubo_data.size = glm::vec2(WIDTH, HEIGHT);

	unif_block_indx = glGetUniformBlockIndex(compProgID, "Unifs");
	glUniformBlockBinding(compProgID, unif_block_indx, 0);

	//printf("\n\n%d\n\n", sizeof(unifs2));

	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ubo_data), &ubo_data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

	//Textures
	for (int i = 0; i < MAX_TEXTURES; i++)
		tex_unifs[i] = glGetUniformLocation(compProgID, ("texs[" + std::to_string(i) + "]").c_str());
}

void scene::initWaterScene() {
	isWaterScene = true;
	water_obj = new water(this);
	myObjGroup->addObj(water_obj);
}

void scene::initBallPitScene() {
	isBallPitScene = true;
	ballPit_obj = new ballPit(this);
}

void scene::updateScene() {
	time += 0.2;
	if (isWaterScene) {
		water_obj->update();
		return;
	}
	if (isBallPitScene) {
		ballPit_obj->update();
	}
}

void scene::updateUniforms(GLuint compProgID) {
	
	float fv = fovy * 3.1416 / 180;

	glm::vec3 dir = glm::normalize(lookAt - eye);
	glm::vec3 left = glm::cross(up, dir);

	glm::vec3 ray00 = glm::normalize(0.5f * WIDTH * left + 0.5f * HEIGHT * up + 0.5f * HEIGHT / tan(0.5f * fv) * dir);
	glm::vec3 ray01 = glm::normalize(0.5f * WIDTH * left - 0.5f * HEIGHT * up + 0.5f * HEIGHT / tan(0.5f * fv) * dir);
	glm::vec3 ray10 = glm::normalize(-0.5f * WIDTH * left + 0.5f * HEIGHT * up + 0.5f * HEIGHT / tan(0.5f * fv) * dir);
	glm::vec3 ray11 = glm::normalize(-0.5f * WIDTH * left - 0.5f * HEIGHT * up + 0.5f * HEIGHT / tan(0.5f * fv) * dir);
	
	ubo_data.time = time;
	ubo_data.eye = glm::vec4(eye, 1);
	ubo_data.ray00 = glm::vec4(ray00, 1);
	ubo_data.ray01 = glm::vec4(ray01, 1);
	ubo_data.ray10 = glm::vec4(ray10, 1);
	ubo_data.ray11 = glm::vec4(ray11, 1);

	curTriIndx = 0;
	curSphereIndx = 0;
	curPlaneIndx = 0;
	
	myObjGroup->updateUniforms();

	ubo_data.num_tris = curTriIndx;
	ubo_data.num_spheres = curSphereIndx;
	ubo_data.num_planes = curPlaneIndx;
	ubo_data.num_lights = myLights.size();

	ubo_data.ambLight = glm::vec4(ambLight, 1);
	ubo_data.bgCol = glm::vec4(bgColor, 1);

	for (int i = 0; i < myLights.size(); i++)
		ubo_data.lights[i] = glm::vec4(myLights[i].position, 1);

	for (int i = 0; i < myMaterials.size(); i++) {
		material mat = myMaterials.at(i);
		_material m_res = {
			glm::vec4(mat.diffuseCol, 1),
			glm::vec4(mat.specularCol, 1),
			glm::vec4(mat.reflectiveCol, 1),
			glm::vec4(mat.transparentCol, 1),

			mat.shininess,
			mat.refractionIndex,

			mat.textureIndex
		};

		ubo_data.maters[i] = m_res;
	}

	for (int i = 0; i < textureIDs.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i + 1);
		glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
		glUniform1i(tex_unifs[i], i + 1);
	}
	glActiveTexture(GL_TEXTURE0);
	
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ubo_data), &ubo_data);
	//glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void scene::parse(const char *filename)
{
	//some default values in case parsing fails
	myObjGroup = NULL;
	bgColor = glm::vec3(0.5, 0.5, 0.5);
	ambLight = glm::vec3(0.5, 0.5, 0.5);
	eye = glm::vec3(0, 0, 0);
	lookAt = glm::vec3(0, 0, -1);
	up = glm::vec3(0, 1, 0);
	fovy = 45;
	file = NULL;
	curline = 1;

	//the file-extension needs to be "ray"
	assert(filename != NULL);

	if (strcmp(filename, "../PhotonMapper/Media/SampleScenes/water.ray") == 0) {
		initWaterScene();
		return;
	}
	if (strcmp(filename, "../PhotonMapper/Media/SampleScenes/ballPit.ray") == 0) {
		initBallPitScene();
		return;
	}
	
	const char *ext = &filename[strlen(filename) - 4];
	// if no ray extention
	if (strcmp(ext, ".ray"))
	{
		printf("ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ::\"%s\"\n", filename);
		assert(!strcmp(ext, ".ray"));
	}
	file = fopen(filename, "r");
	// Fails if no file exists
	if (file == NULL)
	{
		printf("ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ::\"%s\"\n", filename);
		//assert(file != NULL);
	}
	char token[MAX_PARSER_TOKEN_LENGTH];

	//prime the parser pipe
	parse_char = fgetc(file);

	while (getToken(token))
	{
		if (!strcmp(token, "Background"))
			parseBackground();
		else if (!strcmp(token, "Camera"))
			parseCamera();
		else if (!strcmp(token, "Materials"))
			parseMaterials();
		else if (!strcmp(token, "Group"))
			myObjGroup = parseGroup();
		else if (!strcmp(token, "Lights"))
			parseLights();
		else
		{
			std::cout << "Unknown token in parseFile: '" << token <<
				"' at input line " << curline << "\n";
			exit(-1);
		}
	}
}

/* Parse the "Camera" token */
void scene::parseCamera()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	//get the eye, center, and up vectors (similar to gluLookAt)
	getToken(token); assert(!strcmp(token, "eye"));
	eye = readVec3f();

	getToken(token); assert(!strcmp(token, "lookAt"));
	lookAt = readVec3f();

	getToken(token); assert(!strcmp(token, "up"));
	up = readVec3f();

	getToken(token); assert(!strcmp(token, "fovy"));
	fovy = readFloat();

	getToken(token); assert(!strcmp(token, "}"));
}

/* Parses the "Background" token */
void scene::parseBackground()
{
	char token[MAX_PARSER_TOKEN_LENGTH];

	getToken(token); assert(!strcmp(token, "{"));
	while (1)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			break;
		}
		else if (!strcmp(token, "color"))
		{
			bgColor = readVec3f();
		}
		else if (!strcmp(token, "ambientLight"))
		{
			ambLight = readVec3f();
		}
		else
		{
			std::cout << "Unknown token in parseBackground: " << token << "\n";
			assert(0);
		}
	}
}

/* Parses the "Group" token */
rtObjGroup* scene::parseGroup()
{
	char token[MAX_PARSER_TOKEN_LENGTH];


	getToken(token);
	assert(!strcmp(token, "{"));

	/**********************************************/
	/* Instantiate the group object               */
	/**********************************************/
	rtObjGroup *answer = new rtObjGroup();

	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else
		{
			if (!strcmp(token, "Sphere"))
			{
				sphere *sceneElem = parseSphere();
				assert(sceneElem != NULL);
				answer->addObj(sceneElem);
			}
			else if (!strcmp(token, "Triangle"))
			{
				triangle *sceneElem = parseTriangle();
				assert(sceneElem != NULL);
				answer->addObj(sceneElem);
			}
			else
			{
				std::cout << "Unknown token in Group: '" << token << "' at line "
					<< curline << "\n";
				exit(0);
			}
		}
	}

	/* Return the group */
	return answer;
}

/* Parse the "Sphere" token */
sphere* scene::parseSphere()
{
	char token[MAX_PARSER_TOKEN_LENGTH];

	getToken(token); assert(!strcmp(token, "{"));
	getToken(token); assert(!strcmp(token, "materialIndex"));
	int sphere_material_index = readInt();

	getToken(token); assert(!strcmp(token, "center"));
	glm::vec3 center = readVec3f();
	getToken(token); assert(!strcmp(token, "radius"));
	float radius = readFloat();
	getToken(token); assert(!strcmp(token, "}"));

	std::printf("Sphere:\n\tCenter: %s\n", glm::to_string(center).c_str());
	std::printf("\tRadius: %f\n", radius);
	std::printf("\tSphere Material Index: %d\n\n", sphere_material_index);

	/**********************************************/
	/* The call to your own constructor goes here */
	/**********************************************/
	return new sphere(center, radius, sphere_material_index, this);
}

/* Parse out the "Triangle" token */
triangle* scene::parseTriangle()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Parse out vertex information */
	getToken(token); assert(!strcmp(token, "vertex0"));
	glm::vec3 v0 = readVec3f();
	getToken(token); assert(!strcmp(token, "vertex1"));
	glm::vec3 v1 = readVec3f();
	getToken(token); assert(!strcmp(token, "vertex2"));
	glm::vec3 v2 = readVec3f();
	getToken(token); assert(!strcmp(token, "tex_xy_0"));
	float x0 = 0;
	float y0 = 0;
	x0 = readFloat();
	y0 = readFloat();
	getToken(token); assert(!strcmp(token, "tex_xy_1"));
	float x1 = 0;
	float y1 = 0;
	x1 = readFloat();
	y1 = readFloat();
	getToken(token); assert(!strcmp(token, "tex_xy_2"));
	float x2 = 0;
	float y2 = 0;
	x2 = readFloat();
	y2 = readFloat();
	getToken(token); assert(!strcmp(token, "materialIndex"));
	int mat = readInt();

	getToken(token); assert(!strcmp(token, "}"));


	std::printf("Triangle:\n");
	std::printf("\tVertex0: %s tex xy 0 %s\n", glm::to_string(v0).c_str(), glm::to_string(glm::vec2(x0, y0)).c_str());
	std::printf("\tVertex1: %s tex xy 1 %s\n", glm::to_string(v1).c_str(), glm::to_string(glm::vec2(x1, y1)).c_str());
	std::printf("\tVertex1: %s tex xy 1 %s\n", glm::to_string(v2).c_str(), glm::to_string(glm::vec2(x2, y2)).c_str());
	std::printf("\tTriangle Material Index: %d\n\n", mat);

	/**********************************************/
	/* The call to your own constructor goes here */
	/**********************************************/
	return new triangle(v0, v1, v2, x0, x1, x2, y0, y1, y2, mat, this);
}

/* Parse the "Materials" token */
void scene::parseMaterials()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char texname[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Loop over each Material */
	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else if (!strcmp(token, "Material"))
		{
			getToken(token); assert(!strcmp(token, "{"));
			texname[0] = '\0';
			glm::vec3 diffuseColor(1, 1, 1);
			glm::vec3 specularColor(0, 0, 0);
			float shininess = 1;
			glm::vec3 transparentColor(0, 0, 0);
			glm::vec3 reflectiveColor(0, 0, 0);
			float indexOfRefraction = 1;

			while (1)
			{
				getToken(token);
				if (!strcmp(token, "textureFilename"))
				{
					getToken(token);
					strcpy(texname, token);
				}
				else if (!strcmp(token, "diffuseColor"))
					diffuseColor = readVec3f();
				else if (!strcmp(token, "specularColor"))
					specularColor = readVec3f();
				else if (!strcmp(token, "shininess"))
					shininess = readFloat();
				else if (!strcmp(token, "transparentColor"))
					transparentColor = readVec3f();
				else if (!strcmp(token, "reflectiveColor"))
					reflectiveColor = readVec3f();
				else if (!strcmp(token, "indexOfRefraction"))
					indexOfRefraction = readFloat();
				else
				{
					assert(!strcmp(token, "}"));
					break;
				}
			}

			material temp;

			std::printf("Material:\n", texname);
			temp.diffuseCol = diffuseColor;
			std::printf("\tDiffuse Color: %s\n", glm::to_string(diffuseColor).c_str());
			temp.specularCol = specularColor;
			std::printf("\tSpecular Color: %s\n", glm::to_string(specularColor).c_str());
			temp.shininess = shininess;
			std::printf("\tShininess: %f\n", shininess);
			temp.transparentCol = transparentColor;
			std::printf("\tTransparent Color: %s\n", glm::to_string(transparentColor).c_str());
			temp.reflectiveCol = reflectiveColor;
			std::printf("\tReflective Color: %s\n", glm::to_string(reflectiveColor).c_str());
			temp.refractionIndex = indexOfRefraction;
			std::printf("\tIndex Of Refraction: %f\n", indexOfRefraction);
			std::printf("\tFileName: %s\n\n", texname);
			if (!strcmp(texname, "NULL"))
				temp.textureIndex = -1;
			else
				temp.textureIndex = loadTexture(texname);

			myMaterials.push_back(temp);

		}
	}
}

int scene::loadTexture(char const* texname) {
	//check if this texture has already been loaded
	int existingIndex = -1;
	for (int i = 0; i < textureNames.size(); i++) {
		if (!strcmp(texname, textureNames.at(i).c_str())) {
			existingIndex = i;
			break;
		}
	}

	//if the texture hasn't been loaded yet, we need to load it
	if (existingIndex == -1) {
		int indx = textureIDs.size();
		unsigned int textureID;
		glGenTextures(1, &textureID);
		int width, height, nrComponents;
		unsigned char* data = stbi_load(texname, &width, &height, &nrComponents, 0);
		printf("\nLoading texture... width: %d, height: %d, nrComponents: %d, indx: %d\n\n", width, height, nrComponents, indx);
		if (data)
		{
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			//glActiveTexture(GL_TEXTURE0 + indx + 1);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << texname << std::endl;
			stbi_image_free(data);
		}

		textureIDs.push_back(textureID);
		textureNames.push_back(texname);
		return indx;
	}
	else
	{
		return existingIndex;
	}
}

void scene::parseLights()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char texname[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Loop over each Material */
	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else if (!strcmp(token, "Light"))
		{
			getToken(token); assert(!strcmp(token, "{"));
			texname[0] = '\0';
			glm::vec3 position(0, 0, 0);
			glm::vec3 color(1, 1, 1);

			while (1)
			{
				getToken(token);
				if (!strcmp(token, "position"))
					position = readVec3f();
				else if (!strcmp(token, "color"))
					color = readVec3f();
				else
				{
					assert(!strcmp(token, "}"));
					break;
				}
			}

			/**********************************************/
			/* The call to your own constructor goes here */
			/**********************************************/
			light temp;
			temp.position = position;
			std::printf("Light:\n\tPostion: %s\n", glm::to_string(position).c_str());
			temp.color = color;
			std::printf("\Color: %s\n\n", glm::to_string(color).c_str());
			myLights.push_back(temp);

		}
	}
}

/* consume whitespace */
void scene::eatWhitespace(void)
{
	bool working = true;

	do {
		while (isspace(parse_char))
		{
			if (parse_char == '\n')
			{
				curline++;
			}
			parse_char = fgetc(file);
		}

		if ('#' == parse_char)
		{
			/* this is a comment... eat until end of line */
			while (parse_char != '\n')
			{
				parse_char = fgetc(file);
			}

			curline++;
		}
		else
		{
			working = false;
		}

	} while (working);
}

/* Parse out a single token */
int scene::getToken(char token[MAX_PARSER_TOKEN_LENGTH])
{
	int idx = 0;

	assert(file != NULL);
	eatWhitespace();

	if (parse_char == EOF)
	{
		token[0] = '\0';
		return 0;
	}
	while ((!(isspace(parse_char))) && (parse_char != EOF))
	{
		token[idx] = parse_char;
		idx++;
		parse_char = fgetc(file);
	}

	token[idx] = '\0';
	return 1;
}

/* Reads in a 3-vector */
glm::vec3 scene::readVec3f()
{	
	float a = readFloat();
	float b = readFloat();
	float c = readFloat();
	return glm::vec3(a, b, c);
}

/* Reads in a single float */
float scene::readFloat()
{
	float answer;
	char buf[MAX_PARSER_TOKEN_LENGTH];

	if (!getToken(buf))
	{
		std::cout << "Error trying to read 1 float (EOF?)\n";
		assert(0);
	}

	int count = sscanf(buf, "%f", &answer);
	if (count != 1)
	{
		std::cout << "Error trying to read 1 float\n";
		assert(0);

	}
	return answer;
}

/* Reads in a single int */
int scene::readInt()
{
	int answer;
	char buf[MAX_PARSER_TOKEN_LENGTH];

	if (!getToken(buf))
	{
		std::cout << "Error trying to read 1 int (EOF?)\n";
		assert(0);
	}

	int count = sscanf(buf, "%d", &answer);
	if (count != 1)
	{
		std::cout << "Error trying to read 1 int\n";
		assert(0);

	}
	return answer;
}
