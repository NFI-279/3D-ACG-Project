#include "Graphics\window.h"
#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include "imgui.h"
#include "GUI/GUIManager.h"

void processKeyboardInput();

struct Tower {
	glm::vec3 position;
	glm::vec3 scale;
	bool rotateY;
};
std::vector<Tower> towers;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

Window window("Game Engine", 800, 800);
Camera camera;

// Light
glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(-180.0f, 100.0f, -200.0f);

// Player
glm::vec3 playerPos = glm::vec3(0.0f, -19.0f, 0.0f);
float playerYaw = 0.0f; // rotation Y axis

// Camera
float playerSpeed = 100.0f;
float rotationSpeed = 90.0f;
float camYaw = -90.0f;
float camPitch = -20.0f;
float mouseSensitivity = 0.1f;
float cameraDistance = 12.0f;

//Map Boundaries
float mapMinX = -90.0f;
float mapMaxX = 90.0f;
float mapMinZ = -80.0f;
float mapMaxZ = 65.0f;

GUIManager gui; // Create my instance for the GUI
static bool openMenu = false; // To manage the Insert key press

int main()
{
	gui.Init(window.getWindow());

	glClearColor(0.2f, 0.8f, 1.0f, 1.0f);

	//building and compiling shader program
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

	//Textures
	GLuint tex = loadBMP("Resources/Textures/wood.bmp");
	GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
	GLuint tex3 = loadBMP("Resources/Textures/city1.bmp");
	GLuint tex4 = loadBMP("Resources/Textures/tower1.bmp");

	glEnable(GL_DEPTH_TEST);

	//Test custom mesh loading
	std::vector<Vertex> vert;
	vert.push_back(Vertex());
	vert[0].pos = glm::vec3(10.5f, 10.5f, 0.0f);
	vert[0].textureCoords = glm::vec2(1.0f, 1.0f);

	vert.push_back(Vertex());
	vert[1].pos = glm::vec3(10.5f, -10.5f, 0.0f);
	vert[1].textureCoords = glm::vec2(1.0f, 0.0f);

	vert.push_back(Vertex());
	vert[2].pos = glm::vec3(-10.5f, -10.5f, 0.0f);
	vert[2].textureCoords = glm::vec2(0.0f, 0.0f);

	vert.push_back(Vertex());
	vert[3].pos = glm::vec3(-10.5f, 10.5f, 0.0f);
	vert[3].textureCoords = glm::vec2(0.0f, 1.0f);

	vert[0].normals = glm::normalize(glm::cross(vert[1].pos - vert[0].pos, vert[3].pos - vert[0].pos));
	vert[1].normals = glm::normalize(glm::cross(vert[2].pos - vert[1].pos, vert[0].pos - vert[1].pos));
	vert[2].normals = glm::normalize(glm::cross(vert[3].pos - vert[2].pos, vert[1].pos - vert[2].pos));
	vert[3].normals = glm::normalize(glm::cross(vert[0].pos - vert[3].pos, vert[2].pos - vert[3].pos));

	std::vector<int> ind = { 0, 1, 3,
		1, 2, 3 };

	std::vector<Texture> textures;
	textures.push_back(Texture());
	textures[0].id = tex;
	textures[0].type = "texture_diffuse";

	std::vector<Texture> textures2;
	textures2.push_back(Texture());
	textures2[0].id = tex2;
	textures2[0].type = "texture_diffuse";

	std::vector<Texture> textures3;
	textures3.push_back(Texture());
	textures3[0].id = tex3;
	textures3[0].type = "texture_diffuse";

	std::vector<Texture> textures4;
	textures4.push_back(Texture());
	textures4[0].id = tex4;
	textures4[0].type = "texture_diffuse";


	Mesh mesh(vert, ind, textures3);

	// Create Obj files - easier :)
	MeshLoaderObj loader;
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
	Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
	Mesh plane = loader.loadObj("Resources/Models/plane.obj", textures3);
	//Mesh player = loader.loadObj("Resources/Models/player.obj", textures2);
	Mesh towerMesh = loader.loadObj("Resources/Models/plane.obj", textures4);


	// Create some towers
	//towers.push_back({ glm::vec3(30.0f, -19.5f, 20.0f), glm::vec3(10.0f, 80.0f, 10.0f) });
	//towers.push_back({ glm::vec3(-40.0f, -19.5f, -10.0f), glm::vec3(15.0f, 120.0f, 15.0f) });
	towers.push_back({ glm::vec3(44.0f, -12.0f, 2.0f), glm::vec3(0.154f, 0.1f, 0.154f), false });
	towers.push_back({ glm::vec3(44.0f, -12.0f, 30.0f), glm::vec3(0.154f, 0.1f, 0.154f), false });
	towers.push_back({ glm::vec3(30.0f, -12.0f, 16.0f), glm::vec3(0.154f, 0.1f, 0.154f), true });
	towers.push_back({ glm::vec3(58.0f, -12.0f, 16.0f), glm::vec3(0.154f, 0.1f, 0.154f), true });


	static double lastX = window.getWidth() / 2.0;
	static double lastY = window.getHeight() / 2.0;

	// Stats counters
	int totalRenderedObjects = 0;

	// Variables FPS Calculation
	float fpsAccumulator = 0.0f;
	int fpsFrameCount = 0;
	float displayFPS = 0.0f;

	//check if we close the window or press the escape button
	while (!window.isPressed(GLFW_KEY_ESCAPE) &&
		glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();
		totalRenderedObjects = 0;

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// fps calculation
		fpsAccumulator += deltaTime;
		fpsFrameCount++;

		// Update the FPS display only every 1 second
		if (fpsAccumulator >= 1.0f)
		{
			displayFPS = fpsFrameCount / fpsAccumulator;
			fpsFrameCount = 0;
			fpsAccumulator = 0.0f;
		}

		float velocity = playerSpeed * deltaTime;
		float rotVelocity = rotationSpeed * deltaTime;

		// Mouse Input
		double xpos, ypos;
		glfwGetCursorPos(window.getWindow(), &xpos, &ypos);
		float xoffset = (xpos - lastX) * mouseSensitivity;
		float yoffset = (lastY - ypos) * mouseSensitivity;
		lastX = xpos;
		lastY = ypos;

		ImGuiIO& io = ImGui::GetIO();
		//if (!io.WantCaptureMouse && !gui.showGUI)
		//{
			camYaw += xoffset;
			camPitch += yoffset;
			camPitch = glm::clamp(camPitch, -60.0f, 10.0f);
			//}

		// Camera Directions
		glm::vec3 camForward = camera.getCameraViewDirection();
		camForward.y = 0.0f;
		camForward = glm::normalize(camForward);
		glm::vec3 camRight = glm::normalize(glm::cross(camForward, glm::vec3(0, 1, 0)));

		// Player Movement
		glm::vec3 moveDir(0.0f);
		//if (!gui.showGUI)
		//{
			if (window.isPressed(GLFW_KEY_W)) moveDir += camForward;
			if (window.isPressed(GLFW_KEY_S)) moveDir -= camForward;
			if (window.isPressed(GLFW_KEY_A)) moveDir -= camRight;
			if (window.isPressed(GLFW_KEY_D)) moveDir += camRight;

			if (glm::length(moveDir) > 0.001f)
			{
				moveDir = glm::normalize(moveDir);
				playerPos += moveDir * velocity;
				playerYaw = glm::degrees(atan2(moveDir.z, moveDir.x));
			}
			//}

		// Ground 
		playerPos.x = glm::clamp(playerPos.x, mapMinX, mapMaxX);
		playerPos.y = -19.5f;
		playerPos.z = glm::clamp(playerPos.z, mapMinZ, mapMaxZ);

		// Camera orbit
		glm::vec3 offset;
		offset.x = cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
		offset.y = sin(glm::radians(camPitch));
		offset.z = sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));

		glm::vec3 cameraPos = playerPos - offset * cameraDistance;
		camera.setCameraPosition(cameraPos);
		camera.setCameraViewDirection(playerPos - cameraPos);

		//// Code for the light ////

		sunShader.use();

		glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);
		glm::mat4 ViewMatrix = glm::lookAt(camera.getCameraPosition(), camera.getCameraPosition() + camera.getCameraViewDirection(), camera.getCameraUp());

		GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

		glm::mat4 ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, lightPos);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		sun.draw(sunShader);
		totalRenderedObjects++;

		//// End code for the light ////

		shader.use();

		///// Test Obj files for box ////

		GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
		GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");
		ModelMatrix = glm::mat4(1.0f);
		ModelMatrix = glm::translate(ModelMatrix, playerPos);
		ModelMatrix = glm::rotate(
			ModelMatrix,
			glm::radians(playerYaw),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		/*ModelMatrix = glm::scale(
			ModelMatrix,
			glm::vec3(0.0125f)
		);*/
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		box.draw(shader);
		totalRenderedObjects++;

		///// Test plane Obj file //////

		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -20.0f, 0.0f));
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		plane.draw(shader);
		totalRenderedObjects++;

		for (const Tower & tower : towers) { 
			ModelMatrix = glm::mat4(1.0f); 
			ModelMatrix = glm::translate(ModelMatrix, tower.position); 
			ModelMatrix = glm::rotate(ModelMatrix, (90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			if (tower.rotateY) {
				ModelMatrix = glm::rotate(ModelMatrix, (90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			ModelMatrix = glm::scale(ModelMatrix, tower.scale); 
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix; 
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]); 
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]); 
			towerMesh.draw(shader); 
			totalRenderedObjects++; 
		}

		gui.Render(playerPos, window.getWidth(), window.getHeight(), displayFPS, totalRenderedObjects);

		if (window.isPressed(GLFW_KEY_INSERT))
		{
			if (!openMenu)
			{
				gui.showGUI = !gui.showGUI;
				if (!gui.showGUI)
				{
					ImGuiIO& io = ImGui::GetIO();
					io.MouseDown[0] = false;
					io.MouseDown[1] = false;
					io.MouseDown[2] = false;
					io.WantCaptureMouse = false;
				}
				openMenu = true;
			}
		}
		else
		{
			openMenu = false;
		}

		if (gui.changeBackground)
			glClearColor(0.8f, 0.2f, 0.2f, 1.0f);
		else
			glClearColor(0.2f, 0.8f, 1.0f, 1.0f);

		window.update();
	}
	gui.Shutdown();
}

void processKeyboardInput()
{
}