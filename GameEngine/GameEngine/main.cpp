#include <iostream>
#include <string>
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

struct Tree {
	glm::vec3 position;
	float scale;
};
std::vector<Tree> trees;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

Window window("Game Engine", 800, 800);
Camera camera;

// Light
glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(-180.0f, 100.0f, -200.0f);

// --- PLAYER HEALTH SYSTEM ---
float playerHP = 100.0f;
float maxPlayerHP = 100.0f;
float damageRadius = 1.5f;     // Distance to take damage
float damageFlashTimer = 0.0f; // For visual feedback
// ----------------------------


// Player
glm::vec3 playerPos = glm::vec3(1.0f, -19.0f, 1.0f);
float playerYaw = 0.0f; // rotation Y axis
// Legs animation
float walkCycle = 0.0f; // accumulates movement for leg animation
float walkSpeedFactor = 5.0f; // controls leg swing speed
float walkAmplitude = 0.2f;   // leg swing amplitude

// Monsters
struct Monster {
	glm::vec3 position;
	float bodyScale;
	float yaw;
	float walkCycle = 0.0f; // accumulates movement
};

std::vector<Monster> monsters;
float monsterSpeed = 5.0f;
float chaseDistance = 10.0f; // maximum distance to start chasing
float spawnInterval = 3.0f;       // seconds between monster spawns
float timeSinceLastSpawn = 0.0f;  // accumulates time
float spawnDistance = 10.0f;      // min distance from player

// Camera
float playerSpeed = 10.0f;
float rotationSpeed = 90.0f;
float camYaw = -90.0f;
float camPitch = -20.0f;
float mouseSensitivity = 0.1f;
float cameraDistance = 12.0f;

//Map Boundaries
float mapMinX = -360.0f;
float mapMaxX = 0.0f;
float mapMinZ = 0.0f;
float mapMaxZ = 270.0f;

//Plane
float baseX = 0.0f;
float baseZ = 0.0f;
float stepX = -179.5f;
float stepZ = 136.85f;
float groundY = -20.0f;

GUIManager gui; // Create my instance for the GUI
static bool openMenu = false; // To manage the Insert key press

int main()
{
	gui.Init(window.getWindow());

	glClearColor(0.2f, 0.8f, 1.0f, 1.0f);

	//building and compiling shader program
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");
	Shader bodyShader("Shaders/vertex_shader.glsl", "Shaders/body_fragment_shader.glsl");

	//Textures
	GLuint tex = loadBMP("Resources/Textures/wood.bmp");
	GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
	GLuint tex3 = loadBMP("Resources/Textures/city1.bmp");
	GLuint tex4 = loadBMP("Resources/Textures/tower1.bmp");
	GLuint bodyTex = loadBMP("Resources/Textures/dirty.bmp");

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

	std::vector<Texture> bodyTextures;
	bodyTextures.push_back(Texture());
	bodyTextures[0].id = bodyTex;
	bodyTextures[0].type = "texture_diffuse";


	Mesh mesh(vert, ind, textures3);

	// Create Obj files - easier :)
	MeshLoaderObj loader;
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
	Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
	Mesh plane = loader.loadObj("Resources/Models/plane.obj", textures3);
	//Mesh player = loader.loadObj("Resources/Models/player.obj", textures2);
	Mesh towerMesh = loader.loadObj("Resources/Models/plane.obj", textures4);
	//Mesh treeMesh = loader.loadObj("Resources/Models/tree1.obj");
	Mesh bodyBox = loader.loadObj("Resources/Models/cube.obj", bodyTextures);

	// Create wall for tower
	towers.push_back({ glm::vec3(-34.0f, 0.0f, 39.0f), glm::vec3(0.154f, 0.1f, 0.154f), false });
	towers.push_back({ glm::vec3(-34.0f, 0.0f, 66.0f), glm::vec3(0.154f, 0.1f, 0.154f), false });
	towers.push_back({ glm::vec3(-34.5f, 0.0f, 66.5f), glm::vec3(0.154f, 0.1f, 0.154f), true });
	towers.push_back({ glm::vec3(-61.5f, 0.0f, 66.5f), glm::vec3(0.154f, 0.1f, 0.154f), true });

	// Trees
	trees.push_back({ glm::vec3(10.0f, -19.5f, 10.0f), 1.5f });
	trees.push_back({ glm::vec3(-20.0f, -19.5f, 5.0f), 2.0f });
	trees.push_back({ glm::vec3(30.0f, -19.5f, -15.0f), 1.2f });

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
			walkCycle += glm::length(moveDir) * velocity * walkSpeedFactor;
		}
		//}
		// 
		// Random monster spawning parameters
		//float spawnDistance = 20.0f; // min distance from player
		//float spawnChance = 0.01f;   // chance per frame to spawn a monster

		// Update the spawn timer
		timeSinceLastSpawn += deltaTime;

		if (timeSinceLastSpawn >= spawnInterval)
		{
			timeSinceLastSpawn = 0.0f;

			Monster m;
			float offsetX = ((float)rand() / RAND_MAX - 0.5f) * spawnDistance * 2.0f;
			float offsetZ = ((float)rand() / RAND_MAX - 0.5f) * spawnDistance * 2.0f;

			m.position = glm::vec3(
				glm::clamp(playerPos.x + offsetX, mapMinX, mapMaxX),
				playerPos.y,
				glm::clamp(playerPos.z + offsetZ, mapMinZ, mapMaxZ)
			);
			m.bodyScale = 1.0f;
			monsters.push_back(m);
		}


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
		glUniform1i(glGetUniformLocation(shader.getId(), "isTree"), 0);

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

		/*box.draw(shader);
		totalRenderedObjects++;*/

		///// Test plane Obj file //////
		for (int x = 0; x < 2; x++)
		{
			for (int z = 0; z < 2; z++)
			{
				ModelMatrix = glm::mat4(1.0f);
				ModelMatrix = glm::translate(ModelMatrix,
					glm::vec3(baseX + x * stepX, groundY - (z * 0.01f), baseZ + z * stepZ));
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				plane.draw(shader);
				totalRenderedObjects++;
			}
		}

		for (const Tower& tower : towers) {
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

		for (const Tree& tree : trees)
		{
			glUniform1i(glGetUniformLocation(shader.getId(), "isTree"), 1);
			glUniform1f(glGetUniformLocation(shader.getId(), "trunkHeight"), -17.0f);
			glUniform3f(glGetUniformLocation(shader.getId(), "trunkColor"), 0.25f, 0.15f, 0.08f);
			glUniform3f(glGetUniformLocation(shader.getId(), "leavesColor"), 0.12f, 0.35f, 0.18f);

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, tree.position);
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(tree.scale));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

			//treeMesh.draw(shader);
			totalRenderedObjects++;
		}

		glUniform1i(glGetUniformLocation(shader.getId(), "isTree"), 0);

		/// SIMPLE HUMAN MODEL ///

		bodyShader.use();
		//glUniform3f(glGetUniformLocation(bodyShader.getId(), "bodyColor"), 0.396f, 0.502f, 0.341f); // dark green
		//glUniform3f(glGetUniformLocation(bodyShader.getId(), "bodyColor"), 0.0f, 1.0f, 0.0f); // all green (maybe for monsters?)
		glUniform3f(glGetUniformLocation(bodyShader.getId(), "bodyColor"), 0.6f, 0.486f, 0.431f); // texture is kind of red-ish
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bodyTex);
		glUniform1i(glGetUniformLocation(bodyShader.getId(), "bodyTexture"), 0);

		// Head (static)
		{
			// glUniform3f(glGetUniformLocation(bodyShader.getId(), "bodyColor"), 1.0f, 0.922f, 0.812f); // skin color

			glm::mat4 headModel = glm::mat4(1.0f);
			headModel = glm::translate(headModel, playerPos);
			headModel = glm::rotate(headModel, glm::radians(playerYaw), glm::vec3(0, 1, 0));
			headModel = glm::translate(headModel, glm::vec3(0.0f, 1.35f, -0.10f)); // position above torso
			headModel = glm::scale(headModel, glm::vec3(0.10f, 0.10f, 0.10f));

			glm::mat4 headMVP = ProjectionMatrix * ViewMatrix * headModel;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &headMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &headModel[0][0]);

			bodyBox.draw(bodyShader);
			totalRenderedObjects++;
		}

		// Torso (static)
		{
			glm::mat4 torsoModel = glm::mat4(1.0f);
			torsoModel = glm::translate(torsoModel, playerPos);
			torsoModel = glm::rotate(torsoModel, glm::radians(playerYaw), glm::vec3(0, 1, 0));
			torsoModel = glm::translate(torsoModel, glm::vec3(0.0f, 0.2f, -0.15f)); // position above legs
			torsoModel = glm::scale(torsoModel, glm::vec3(0.15f, 0.25f, 0.12f)); // width, height, depth

			glm::mat4 torsoMVP = ProjectionMatrix * ViewMatrix * torsoModel;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &torsoMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &torsoModel[0][0]);

			bodyBox.draw(bodyShader);
			totalRenderedObjects++;
		}

		// Arms with swinging animation

		// Left arm
		{
			glm::mat4 armModel = glm::mat4(1.0f);
			armModel = glm::translate(armModel, playerPos);
			armModel = glm::rotate(armModel, glm::radians(playerYaw), glm::vec3(0, 1, 0));
			armModel = glm::translate(armModel, glm::vec3(-0.5f, 0.2f, 0.0f)); // offset from torso

			//armModel = glm::rotate(armModel, glm::radians(180.0f), glm::vec3(1, 0, 0)); // flip the arm

			// Pivot at shoulder for rotation
			armModel = glm::translate(armModel, glm::vec3(0.0f, 0.0f, 0.0f));
			armModel = glm::rotate(armModel, -sin(walkCycle) * glm::radians(400.0f), glm::vec3(1, 0, 0));
			armModel = glm::translate(armModel, glm::vec3(0.0f, 0.0f, 0.0f));

			armModel = glm::scale(armModel, glm::vec3(0.04f, 0.25f, 0.04f)); // width, height, depth

			glm::mat4 armMVP = ProjectionMatrix * ViewMatrix * armModel;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &armMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &armModel[0][0]);

			bodyBox.draw(bodyShader);
			totalRenderedObjects++;
		}

		// Right arm
		{
			glm::mat4 armModel = glm::mat4(1.0f);
			armModel = glm::translate(armModel, playerPos);
			armModel = glm::rotate(armModel, glm::radians(playerYaw), glm::vec3(0, 1, 0));
			armModel = glm::translate(armModel, glm::vec3(0.5f, 0.2f, 0.0f)); // offset from torso

			// Pivot at shoulder for rotation
			armModel = glm::translate(armModel, glm::vec3(0.0f, -0.25f, 0.0f));
			armModel = glm::rotate(armModel, sin(walkCycle) * glm::radians(400.0f), glm::vec3(1, 0, 0));
			armModel = glm::translate(armModel, glm::vec3(0.0f, 0.25f, 0.0f));

			armModel = glm::scale(armModel, glm::vec3(0.04f, 0.25f, 0.04f)); // width, height, depth

			glm::mat4 armMVP = ProjectionMatrix * ViewMatrix * armModel;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &armMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &armModel[0][0]);

			bodyBox.draw(bodyShader);
			totalRenderedObjects++;
		}


		// Legs with swinging animation

		// Left leg
		{
			glm::mat4 legModel = glm::mat4(1.0f);
			legModel = glm::translate(legModel, playerPos);
			legModel = glm::rotate(legModel, glm::radians(playerYaw), glm::vec3(0, 1, 0));
			legModel = glm::translate(legModel, glm::vec3(0.2f, -0.5f, 0.0f)); // hip offset

			legModel = glm::translate(legModel, glm::vec3(0.0f, 0.1f, 0.0f)); // pivot to top of leg
			legModel = glm::rotate(legModel, sin(walkCycle) * glm::radians(360.0f), glm::vec3(1, 0, 0));
			legModel = glm::translate(legModel, glm::vec3(0.0f, -0.1f, 0.0f)); // move pivot back

			legModel = glm::scale(legModel, glm::vec3(0.05f, 0.2f, 0.05f));

			glm::mat4 legMVP = ProjectionMatrix * ViewMatrix * legModel;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &legMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &legModel[0][0]);

			bodyBox.draw(bodyShader);
			totalRenderedObjects++;
		}

		// Right leg
		{
			glm::mat4 legModel = glm::mat4(1.0f);
			legModel = glm::translate(legModel, playerPos);
			legModel = glm::rotate(legModel, glm::radians(playerYaw), glm::vec3(0, 1, 0));
			legModel = glm::translate(legModel, glm::vec3(-0.2f, -0.5f, 0.0f)); // hip offset opposite

			legModel = glm::translate(legModel, glm::vec3(0.0f, 0.1f, 0.0f));
			legModel = glm::rotate(legModel, -sin(walkCycle) * glm::radians(360.0f), glm::vec3(1, 0, 0));
			legModel = glm::translate(legModel, glm::vec3(0.0f, -0.1f, 0.0f));

			legModel = glm::scale(legModel, glm::vec3(0.05f, 0.2f, 0.05f));

			glm::mat4 legMVP = ProjectionMatrix * ViewMatrix * legModel;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &legMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &legModel[0][0]);

			bodyBox.draw(bodyShader);
			totalRenderedObjects++;
		}

		glUniform3f(glGetUniformLocation(bodyShader.getId(), "bodyColor"), 0.0f, 1.0f, 0.0f);

		for (Monster& m : monsters) {
			glm::vec3 dir = playerPos - m.position;
			float distance = glm::length(dir);

			// --- DAMAGE LOGIC START ---
			if (distance < damageRadius) {
				// 10% Damage per second
				float damage = (maxPlayerHP * 0.10f) * deltaTime;
				playerHP -= damage;

				// Trigger Red Flash
				damageFlashTimer = 0.2f;
			}
			// --- DAMAGE LOGIC END ---


			if (distance < chaseDistance && distance > 0.01f) {
				dir = glm::normalize(dir);
				m.position += dir * monsterSpeed * deltaTime;
				m.yaw = glm::degrees(atan2(dir.z, dir.x)) + 90.0f;
				m.walkCycle += glm::length(dir * monsterSpeed * deltaTime) * walkSpeedFactor;
			}

			// Base model transform (position + yaw)
			glm::mat4 humanModel = glm::mat4(1.0f);
			humanModel = glm::translate(humanModel, m.position);
			humanModel = glm::rotate(humanModel, glm::radians(m.yaw), glm::vec3(0, 1, 0));

			float monsterWalkCycle = m.walkCycle;

			// Head
			{
				glm::mat4 headModel = humanModel;
				headModel = glm::translate(headModel, glm::vec3(0.0f, 1.35f, -0.10f));
				headModel = glm::scale(headModel, glm::vec3(0.10f));

				glm::mat4 headMVP = ProjectionMatrix * ViewMatrix * headModel;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &headMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &headModel[0][0]);

				bodyBox.draw(bodyShader);
				totalRenderedObjects++;
			}

			// Torso
			{
				glm::mat4 torsoModel = humanModel;
				torsoModel = glm::translate(torsoModel, glm::vec3(0.0f, 0.2f, -0.15f));
				torsoModel = glm::scale(torsoModel, glm::vec3(0.15f, 0.25f, 0.12f));

				glm::mat4 torsoMVP = ProjectionMatrix * ViewMatrix * torsoModel;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &torsoMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &torsoModel[0][0]);

				bodyBox.draw(bodyShader);
				totalRenderedObjects++;
			}

			// Arms
			{
				// Left arm
				glm::mat4 leftArm = humanModel;
				leftArm = glm::translate(leftArm, glm::vec3(-0.5f, 0.2f, 0.0f));
				leftArm = glm::rotate(leftArm, -sin(monsterWalkCycle) * glm::radians(400.0f), glm::vec3(1, 0, 0));
				leftArm = glm::scale(leftArm, glm::vec3(0.04f, 0.25f, 0.04f));

				glm::mat4 leftArmMVP = ProjectionMatrix * ViewMatrix * leftArm;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &leftArmMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &leftArm[0][0]);
				box.draw(bodyShader);
				totalRenderedObjects++;

				// Right arm
				glm::mat4 rightArm = humanModel;
				rightArm = glm::translate(rightArm, glm::vec3(0.5f, 0.2f, 0.0f));
				rightArm = glm::rotate(rightArm, sin(monsterWalkCycle) * glm::radians(400.0f), glm::vec3(1, 0, 0));
				rightArm = glm::scale(rightArm, glm::vec3(0.04f, 0.25f, 0.04f));

				glm::mat4 rightArmMVP = ProjectionMatrix * ViewMatrix * rightArm;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &rightArmMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &rightArm[0][0]);
				bodyBox.draw(bodyShader);
				totalRenderedObjects++;
			}

			// Legs
			{
				// Left leg
				glm::mat4 leftLeg = humanModel;
				leftLeg = glm::translate(leftLeg, glm::vec3(0.2f, -0.5f, 0.0f));
				leftLeg = glm::translate(leftLeg, glm::vec3(0.0f, 0.1f, 0.0f));
				leftLeg = glm::rotate(leftLeg, sin(monsterWalkCycle) * glm::radians(360.0f), glm::vec3(1, 0, 0));
				leftLeg = glm::translate(leftLeg, glm::vec3(0.0f, -0.1f, 0.0f));
				leftLeg = glm::scale(leftLeg, glm::vec3(0.05f, 0.2f, 0.05f));

				glm::mat4 leftLegMVP = ProjectionMatrix * ViewMatrix * leftLeg;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &leftLegMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &leftLeg[0][0]);
				bodyBox.draw(bodyShader);
				totalRenderedObjects++;

				// Right leg
				glm::mat4 rightLeg = humanModel;
				rightLeg = glm::translate(rightLeg, glm::vec3(-0.2f, -0.5f, 0.0f));
				rightLeg = glm::translate(rightLeg, glm::vec3(0.0f, 0.1f, 0.0f));
				rightLeg = glm::rotate(rightLeg, -sin(monsterWalkCycle) * glm::radians(360.0f), glm::vec3(1, 0, 0));
				rightLeg = glm::translate(rightLeg, glm::vec3(0.0f, -0.1f, 0.0f));
				rightLeg = glm::scale(rightLeg, glm::vec3(0.05f, 0.2f, 0.05f));

				glm::mat4 rightLegMVP = ProjectionMatrix * ViewMatrix * rightLeg;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &rightLegMVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &rightLeg[0][0]);
				bodyBox.draw(bodyShader);
				totalRenderedObjects++;
			}
		}


		// --- HEALTH & VISUALS UPDATE ---

		// 1. Red Flash Effect
		if (damageFlashTimer > 0.0f) {
			damageFlashTimer -= deltaTime;
			gui.changeBackground = true; // Turn background RED
		}
		else {
			gui.changeBackground = false; // Reset to BLUE
		}

		// 2. Death / Respawn Check
		if (playerHP <= 0.0f) {
			std::cout << ">>> YOU DIED! Respawning... <<<" << std::endl;
			playerHP = maxPlayerHP;
			playerPos = glm::vec3(1.0f, -19.0f, 1.0f); // Reset Position
			monsters.clear(); // Clear enemies
		}

		// 3. Debug Print (Every 1 sec approx, or use ImGui text if available)
		// We will just print if damaged to avoid spam
		if (damageFlashTimer > 0.15f) { // Only print on initial hit frame approx
			std::cout << "HP: " << (int)playerHP << " / " << (int)maxPlayerHP << std::endl;
		}
		// -------------------------------

		gui.Render(playerPos, window.getWidth(), window.getHeight(), displayFPS, totalRenderedObjects);

		// TODO FILIP: Remove when out of testing !!!!
		static bool fPressed = false;
		if (window.isPressed(GLFW_KEY_F)) {
			if (!fPressed) {
				gui.questManager.CompleteCurrentQuest();
				gui.AddLog("[GAME] Quest Objective Completed");
				fPressed = true;
			}
		}
		else {
			fPressed = false;
		}

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