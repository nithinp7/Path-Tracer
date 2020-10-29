
#include <PathTracer.hpp>

#define EPSILON 1e-4f

int main(int argc, char **argv)
{
	std::string fn;

	if (argc < 2)// if not specified, prompt for filename 
	{

		std::printf("Enter a '.ray' file to be rendered, e.g. 'SampleScenes/ballPit.ray'...\n\n");
		char input[999];
		std::printf("Input .ray file: ");
		std::cin >> input;
		fn = input;
	}
	else //otherwise, use the name provided
	{
		/* Open jpeg (reads into memory) */
		char* filename = argv[1];
		fn = filename;
	}

	fn = "../PathTracer/Media/" + fn;
	std::printf("Opening File %s\n", fn.c_str());
	GLFWwindow* window  = NULL;

	// glfw: initialize and configure
	// ------------------------------

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	#ifdef __APPLE__
	    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
	#endif
	// glfw window creation
	// --------------------
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PathTracer", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//Now that the context is ready initialize the scene
	myScene = new scene(fn.c_str(), SCR_WIDTH, SCR_HEIGHT);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_MULTISAMPLE); // Enabled by default on some drivers, but not all so always enable to make sure

	// build and compile shaders
	// -------------------------


	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
							    // positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	char const* skybox_path[] = {
		"../PathTracer/Media/Skyboxes/skybox/right.jpg",
		"../PathTracer/Media/Skyboxes/skybox/left.jpg",
		"../PathTracer/Media/Skyboxes/skybox/top.jpg",
		"../PathTracer/Media/Skyboxes/skybox/bottom.jpg",
		"../PathTracer/Media/Skyboxes/skybox/back.jpg",
		"../PathTracer/Media/Skyboxes/skybox/front.jpg"
	};

	// shader configuration
	// --------------------

	//init compute shader
	compProgram = loadComputeShaderProgram("../PathTracer/Shaders/pathtrace.comp");

	myScene->initUniforms(compProgram);

	unsigned int skyboxID = loadSkybox(skybox_path);
	unsigned int skyboxLoc = glGetUniformLocation(compProgram, "skybox");
	
	//init screen shader
	//screenShader = Shader("../PathTracer/Shaders/screenShader.vert", "../PathTracer/Shaders/screenShader.frag");
	screenShader = Shader("../PathTracer/Shaders/photonmap.vert", "../PathTracer/Shaders/photonmap.frag");
	screenShader.use();
	//screenShader.setInt("screenTexture", 0);

	// Create initial image texture
	glGenTextures(1, &textureColorbuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	//glBindImageTexture(0, textureColorbuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	
	bool raytracing = true;
		
	// render loop
	// -----------
	bool running = true;
	running = !glfwWindowShouldClose(window);

	float time = 0;
	float dt = 0.06;
	bool initialRun = true;
	while (running)
	{
		time += dt;

		// input
		// -----

		processInput(window);

		if (false && initialRun)
		{
			myScene->updateScene();

			glUseProgram(compProgram);

			glm::vec3 eye = myScene->eye;
			glm::vec3 lookAt = myScene->lookAt;
			glm::vec3 up = myScene->up;
			float fovy = myScene->fovy;

			myScene->updateUniforms(compProgram);

			glActiveTexture(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);
			glUniform1i(skyboxLoc, 0);
			glActiveTexture(GL_TEXTURE0);

			glBindImageTexture(0, textureColorbuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

			glDispatchCompute((GLuint)SCR_WIDTH, (GLuint)SCR_HEIGHT, (GLuint)1);

			//glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			// set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			initialRun = false;
		}

		if (initialRun)
		{
			myScene->updateScene();

			screenShader.use();

			glm::vec3 eye = myScene->eye;
			glm::vec3 lookAt = myScene->lookAt;
			glm::vec3 up = myScene->up;
			float fovy = myScene->fovy;

			myScene->updateUniforms(screenShader.ID);

			glActiveTexture(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);
			glUniform1i(skyboxLoc, 0);
			glActiveTexture(GL_TEXTURE0);

			// set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			initialRun = false;
		}
		/* * /
		screenShader.use();
		// Bind our texture we are creating
		//glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(quadVAO);
		//glBindTexture(GL_TEXTURE_3D, textureColorbuffer);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		*/

		glfwSwapBuffers(window);
		glfwPollEvents();

		running = !glfwWindowShouldClose(window);
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
    glDeleteBuffers(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glfwTerminate();
    
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
//   The movement of the boxes is still here.  Feel free to use it or take it out
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	// Escape Key quits
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W))
		myScene->eye += 3.0f*glm::normalize(myScene->lookAt - myScene->eye);
	if (glfwGetKey(window, GLFW_KEY_S)) 
		myScene->eye += 3.0f*glm::normalize(myScene->eye - myScene->lookAt);
	// P Key saves the image
	//if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		//stbi_write_png("../PathTracer/out.png", SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3)*SCR_WIDTH);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);

}

// Update the current texture in image and sent the data to the textureColorbuffer
void update_image_texture()
{
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int loadSkybox(char const* path[])
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	unsigned char* data;
	for (unsigned int i = 0; i < 6; i++)
	{
		data = stbi_load(path[i], &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << path[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

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
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

GLuint loadShader(std::string path, GLenum shaderType)
{
	//Open file
	GLuint shaderID = 0;
	std::string shaderString;
	std::ifstream sourceFile(path.c_str());

	//Source file loaded
	if (sourceFile)
	{
		//Get shader source
		shaderString.assign((std::istreambuf_iterator< char >(sourceFile)), std::istreambuf_iterator< char >());

		//Create shader ID
		shaderID = glCreateShader(shaderType);

		//Set shader source
		const GLchar* shaderSource = shaderString.c_str();
		glShaderSource(shaderID, 1, (const GLchar**)&shaderSource, NULL);

		//Compile shader source
		glCompileShader(shaderID);

		//Check shader for errors
		GLint shaderCompiled = GL_FALSE;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled);
		if (shaderCompiled != GL_TRUE)
		{
			//printf("Unable to compile shader %d!\n\nSource:\n%s\n", shaderID, shaderSource);
			printf("Unable to compile shader %d!\n", shaderID);

			//printShaderLog(shaderID);
			GLint maxLength = 0;
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetShaderInfoLog(shaderID, maxLength, &maxLength, &errorLog[0]);

			std::string s = std::string(errorLog.begin(), errorLog.end());

			printf("Shader Error Log: %s\n", s.c_str());

			glDeleteShader(shaderID);
			shaderID = 0;

			exit(-1);
		}
	}
	else
	{
		printf("Unable to open file %s\n", path.c_str());
	}

	return shaderID;
}

GLuint loadComputeShaderProgram(std::string path) 
{
	GLuint cshader = loadShader(path, GL_COMPUTE_SHADER);
	GLuint prog = glCreateProgram();
	glAttachShader(prog, cshader);
	glLinkProgram(prog);

	GLint isLinked = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(prog, maxLength, &maxLength, &infoLog[0]);

		// The program is useless now. So delete it.
		glDeleteProgram(prog);
		
		printf("%s", infoLog.data());

		exit(-1);
		// Provide the infolog in whatever manner you deem best.
		// Exit with failure.
	}

	return prog;
}
