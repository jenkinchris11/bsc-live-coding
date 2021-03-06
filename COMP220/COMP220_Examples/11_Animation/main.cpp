//main.cpp - defines the entry point of the application

#include "main.h"

int main(int argc, char* args[])
{
	//Initialises the SDL Library, passing in SDL_INIT_VIDEO to only initialise the video subsystems
	//https://wiki.libsdl.org/SDL_Init
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		//Display an error message box
		//https://wiki.libsdl.org/SDL_ShowSimpleMessageBox
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, SDL_GetError(), "SDL_Init failed", NULL);
		return 1;
	}

	//Create a window, note we have to free the pointer returned using the DestroyWindow Function
	//https://wiki.libsdl.org/SDL_CreateWindow
	SDL_Window* window = SDL_CreateWindow("SDL2 Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	//Checks to see if the window has been created, the pointer will have a value of some kind
	if (window == nullptr)
	{
		//Show error
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, SDL_GetError(), "SDL_CreateWindow failed", NULL);
		//Close the SDL Library
		//https://wiki.libsdl.org/SDL_Quit
		SDL_Quit();
		return 1;
	}

	//lets ask for a 3.2 core profile version of OpenGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GLContext GL_Context = SDL_GL_CreateContext(window);
	if (GL_Context == nullptr)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, SDL_GetError(), "SDL GL Create Context failed", NULL);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	//Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, (char*)glewGetErrorString(glewError), "GLEW Init Failed", NULL);
	}

	std::vector<Mesh*> meshes;
	Joint *pSkelton=nullptr;
	loadMeshFromFile("Ethan.FBX", meshes, &pSkelton);
	loadAnimationFromFile("HumanoidIdle.fbx", nullptr);
	loadAnimationFromFile("HumanoidWalk.fbx", nullptr);
	//GLuint textureID = loadTextureFromFile("Tank1DF.png");

	pSkelton->displayToConsole();
	vec3 objectPosition = vec3(0.0f,0.0f,0.0f);
	vec3 objectScale = vec3(1.0f, 1.0f, 1.0f);
	vec3 objectRotation = vec3(0.0f, glm::radians(180.0f), 0.0f);

	
	mat4 translationMatrix = translate(objectPosition);
	mat4 scaleMatrix = scale(objectScale);
	mat4 rotationMatrix= rotate(objectRotation.x, vec3(1.0f, 0.0f, 0.0f))*rotate(objectRotation.y, vec3(0.0f, 1.0f, 0.0f))*rotate(objectRotation.z, vec3(0.0f, 0.0f, 1.0f));

	mat4 modelMatrix = translationMatrix*rotationMatrix*scaleMatrix;

	vec3 cameraPosition = vec3(0.0f, 100.0f, -200.0f);
	vec3 cameraTarget = vec3(0.0f, 10.0f, 0.0f);
	vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

	mat4 viewMatrix = lookAt(cameraPosition, cameraTarget, cameraUp);

	mat4 projectionMatrix = perspective(radians(90.0f), float(800 / 640), 0.1f, 1000.0f);

	//Light
	vec4 ambientLightColour = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec3 lightDirection = vec3(0.0f, 0.0f, -1.0f);
	vec4 diffuseLightColour = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 specularLightColour = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	//Material
	vec4 ambientMaterialColour = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 diffuseMaterialColour = vec4(0.0f, 0.6f, 0.0f, 1.0f);
	vec4 specularMaterialColour = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float specularPower = 25.0f;

	GLuint programID = LoadShaders("lightingVert.glsl", "lightingFrag.glsl");

	GLint fragColourLocation=glGetUniformLocation(programID, "fragColour");
	if (fragColourLocation < 0)
	{
		printf("Unable to find %s uniform\n", "fragColour");
	}

	static const GLfloat fragColour[] = { 0.0f,1.0f,0.0f,1.0f };

	GLint currentTimeLocation= glGetUniformLocation(programID, "time");
	GLint modelMatrixLocation = glGetUniformLocation(programID, "modelMatrix");
	GLint viewMatrixLocation = glGetUniformLocation(programID, "viewMatrix");
	GLint projectionMatrixLocation = glGetUniformLocation(programID, "projectionMatrix");
	GLint textureLocation = glGetUniformLocation(programID, "baseTexture");
	GLint cameraPositionLocation = glGetUniformLocation(programID, "cameraPosition");
	
	GLint lightDirectionLocation = glGetUniformLocation(programID, "lightDirection");
	GLint ambientLightColourLocation = glGetUniformLocation(programID, "ambientLightColour");
	GLint diffuseLightColourLocation = glGetUniformLocation(programID, "diffuseLightColour");
	GLint specularLightColourLocation = glGetUniformLocation(programID, "specularLightColour");

	GLint ambientMaterialColourLocation = glGetUniformLocation(programID, "ambientMaterialColour");
	GLint diffuseMaterialColourLocation = glGetUniformLocation(programID, "diffuseMaterialColour");
	GLint specularMaterialColourLocation= glGetUniformLocation(programID, "specularMaterialColour");
	GLint specularPowerLocation = glGetUniformLocation(programID, "specularPower");

	glEnable(GL_DEPTH_TEST);
	int lastTicks = SDL_GetTicks();
	int currentTicks = SDL_GetTicks();


	//Event loop, we will loop until running is set to false, usually if escape has been pressed or window is closed
	bool running = true;
	//SDL Event structure, this will be checked in the while loop
	SDL_Event ev;
	while (running)
	{
		//Poll for the events which have happened in this frame
		//https://wiki.libsdl.org/SDL_PollEvent
		while (SDL_PollEvent(&ev))
		{
			//Switch case for every message we are intereted in
			switch (ev.type)
			{
				//QUIT Message, usually called when the window has been closed
			case SDL_QUIT:
				running = false;
				break;
				//KEYDOWN Message, called when a key has been pressed down
			case SDL_KEYDOWN:
				//Check the actual key code of the key that has been pressed
				switch (ev.key.keysym.sym)
				{
					//Escape key
				case SDLK_ESCAPE:
					running = false;
					break;
				}
			}
		}

		currentTicks = SDL_GetTicks();
		float deltaTime = (float)(currentTicks - lastTicks) / 1000.0f;

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, textureID);

		glUseProgram(programID);

		glUniform4fv(fragColourLocation, 1, fragColour);
		glUniform1f(currentTimeLocation, (float)(currentTicks)/1000.0f);
		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, value_ptr(modelMatrix));
		glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, value_ptr(viewMatrix));
		glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, value_ptr(projectionMatrix));

		glUniform3fv(cameraPositionLocation, 1, value_ptr(cameraPosition));

		glUniform1i(textureLocation, 0);

		glUniform3fv(lightDirectionLocation, 1, value_ptr(lightDirection));
		glUniform4fv(ambientLightColourLocation, 1, value_ptr(ambientLightColour));
		glUniform4fv(diffuseLightColourLocation, 1, value_ptr(diffuseLightColour));
		glUniform4fv(specularLightColourLocation, 1, value_ptr(specularLightColour));

		glUniform4fv(ambientMaterialColourLocation, 1, value_ptr(ambientMaterialColour));
		glUniform4fv(diffuseMaterialColourLocation, 1, value_ptr(diffuseMaterialColour));
		glUniform4fv(specularMaterialColourLocation, 1, value_ptr(specularMaterialColour));
		glUniform1f(specularPowerLocation, specularPower);

		for (Mesh *pMesh : meshes)
		{
			pMesh->render();
		}
		SDL_GL_SwapWindow(window);

		lastTicks = currentTicks;
	}

	if (pSkelton) {
		pSkelton->destory();
		delete pSkelton;
		pSkelton = nullptr;
	}
	
	auto iter = meshes.begin();
	while (iter != meshes.end())
	{
		if ((*iter))
		{
			delete (*iter);
			iter = meshes.erase(iter);
		}
		else
		{
			iter++;
		}
	}

	meshes.clear();
	//glDeleteTextures(1, &textureID);
	glDeleteProgram(programID);

	SDL_GL_DeleteContext(GL_Context);
	//Destroy the window and quit SDL2, NB we should do this after all cleanup in this order!!!
	//https://wiki.libsdl.org/SDL_DestroyWindow
	SDL_DestroyWindow(window);
	//https://wiki.libsdl.org/SDL_Quit
	SDL_Quit();

	return 0;
}