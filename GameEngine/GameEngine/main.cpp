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
#include "Mountain/mountain.h"

struct Wall {
	glm::vec3 localPos;   
	bool rotateY;      
};
std::vector<Wall> baseWalls = {
	{ glm::vec3(-34.0f, 0.0f, 39.0f), false },
	{ glm::vec3(-34.0f, 0.0f, 66.5f), false },
	{ glm::vec3(-34.0f, 0.0f, 66.5f), true  },
	{ glm::vec3(-61.5f, 0.0f, 66.5f), true  }
};

float step = 38.0f;
glm::vec3 origins[4] = {
	glm::vec3(76.0f, 0.0f, 0.0f),   // first block
	glm::vec3(76.0f, 0.0f, 137.0f),   // second block
	glm::vec3(255.0f, 0.0f, 0.0f),   // third block
	glm::vec3(255.0f, 0.0f, 137.0f)   // fourth block
};

struct Building {        
	std::vector<Wall> walls;   // 4 walls
	float minX, maxX, minZ, maxZ; // coords for collision
};
std::vector<Building> buildings;
std::vector<glm::ivec2> layoutBuildings = {
	glm::ivec2{0, 0},  
	glm::ivec2{0, 1},  
	glm::ivec2{1, 0},  
	glm::ivec2{1, 1},  
	glm::ivec2{2, 0},  
	glm::ivec2{2, 1},
};

float stepTilex = 38.0f;
float stepTilez = 37.0f;
glm::vec3 terrainOrigins[4] = {
	glm::vec3(5.0f,  -19.9f,  2.0f),   // area 0
	glm::vec3(-174.5f,-19.9f,  2.0f),   // area 2
	glm::vec3(5.0f,  -19.9f, 139.5f),  // area 1
	glm::vec3(-174.5f,-19.9f, 139.5f)   // area 3
};

struct TerrainTile {
	glm::vec3 position;
	float scaleX = 0.153f;
	float scaleZ = 0.193f;
};
std::vector<TerrainTile> terrainTiles;

glm::vec3 originTile = glm::vec3(5.0f, -19.9f, 2.0f);
std::vector<glm::ivec2> layoutTiles = {
	glm::ivec2{0, 0},
	glm::ivec2{1, 0},
	glm::ivec2{2, 0},
	glm::ivec2{3, 0},
	glm::ivec2{4, 0},
	glm::ivec2{4, 1},
	glm::ivec2{4, 2},
	glm::ivec2{4, 3},
	glm::ivec2{0, 1},
	glm::ivec2{0, 2},
	glm::ivec2{0, 3},
	glm::ivec2{1, 3},
	glm::ivec2{2, 3},
	glm::ivec2{3, 3},
};

const float BASE_HEIGHT = -20.0f;
const float MOUNTAIN_BUFFER = 100.0f;
const float MOUNTAIN_RAMP = 280.0f;
const float MAX_MOUNTAIN_HEIGHT = 480.0f;
const float PADDING = 220.0f;

struct Tree {
	glm::vec3 position;
	float scale;
	int hitCount = 0;
	float hitCooldown = 0.0f;
	bool alive = true;
	bool destructible = false;
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
int playerScore = 0;

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
// --- SYRINGE SYSTEM ---
struct GarbageItem {
	glm::vec3 position;
	float rotationY;
};

std::vector<GarbageItem> garbageItems;

int playerBackpackCount = 0;   // Current trash (0 to 5)
int playerAntidoteCount = 0;
const int MAX_BACKPACK = 5;

float itemSpawnTimer = 0.0f;
const float ITEM_SPAWN_INTERVAL = 5.0f; // Spawns faster now
const float ITEM_COLLECT_RADIUS = 2.0f;

float maxKillDistance = 10.0f; // maximum distance to kill
bool IsMonsterTargeted(const Monster& m, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float radius = 0.5f)
{
	glm::vec3 target = m.position + glm::vec3(0.0f, 1.5f, 0.0f);
	glm::vec3 oc = target - rayOrigin;
	float t = glm::dot(oc, rayDir);          // distance along the ray
	if (t < 0.0f || t > maxKillDistance)
		return false;

	glm::vec3 closestPoint = rayOrigin + rayDir * t;
	float distanceToMonster = glm::length(target - closestPoint);
	return distanceToMonster <= radius;
}

bool IsTreeTargeted(const Tree& tree, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float radius = 1.1f)
{
	float maxDistance = 5.0f;
	glm::vec3 target = tree.position + glm::vec3(0.0f, tree.scale * 1.2f, 0.0f);
	glm::vec3 oc = target - rayOrigin;
	float t = glm::dot(oc, rayDir);
	if (t < 0.0f || t > maxDistance)
		return false;

	glm::vec3 closestPoint = rayOrigin + rayDir * t;
	float distanceToTree = glm::length(target - closestPoint);
	return distanceToTree <= radius * tree.scale;
}

float monsterSpeed = 5.0f;
float chaseDistance = 10.0f; // maximum distance to start chasing
float spawnInterval = 3.0f;       // seconds between monster spawns
float timeSinceLastSpawn = 0.0f;  // accumulates time
float spawnDistance = 10.0f;      // min distance from player

// Camera
float playerSpeed = 20.0f;
float rotationSpeed = 90.0f;
float camYaw = -90.0f;
float camPitch = -20.0f;
float mouseSensitivity = 0.1f;
float cameraDistance = 12.0f;

//Map Boundaries
float townMinX = -360.0f;
float townMaxX = 0.0f;
float townMinZ = 0.0f;
float townMaxZ = 270.0f;

//Player Boundaries 
float mapMinX = -460.0f;
float mapMaxX = 100.0f;
float mapMinZ = -100.0f;
float mapMaxZ = 380.0f;

//Plane
float baseX = 0.0f;
float baseZ = 0.0f;
float stepX = -179.5f;
float stepZ = 136.85f;
float groundY = -20.0f;

GUIManager gui; // Create my instance for the GUI
static bool openMenu = false; // To manage the Insert key press

void spawnBuilding(const glm::vec3& origin);
bool collidesWithBuildings(const glm::vec3& p);
void spawnTrees(const std::vector<TerrainTile>& terrainTiles, std::vector<Tree>& trees);
void drawtree3D(Shader& shader, Mesh& trunkMesh, Mesh& leavesMesh, const glm::mat4& projection, const glm::mat4& view,
		glm::mat4 model, int depth, int segments, float segmentLength, float radius);
bool collidesWithTrees(const glm::vec3& p);
void processKeyboardInput();

int main()
{
	gui.Init(window.getWindow());

	// HIDE CURSOR 
	glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	

	glClearColor(0.2f, 0.8f, 1.0f, 1.0f);

	//building and compiling shader program
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");
	Shader bodyShader("Shaders/vertex_shader.glsl", "Shaders/body_fragment_shader.glsl");

	//Textures
	GLuint tex = loadBMP("Resources/Textures/wood.bmp");
	GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
	GLuint tex3 = loadBMP("Resources/Textures/city.bmp");
	GLuint tex4 = loadBMP("Resources/Textures/tower.bmp");
	GLuint tex5 = loadBMP("Resources/Textures/terrain.bmp");
	GLuint tex6 = loadBMP("Resources/Textures/plane.bmp");

	GLuint trunkTex = loadBMP("Resources/Textures/trunk.bmp");
	GLuint leavesTex = loadBMP("Resources/Textures/leaves.bmp");

	GLuint bodyTex = loadBMP("Resources/Textures/dirty.bmp");
	GLuint mountainTex = loadBMP("Resources/Textures/mountain.bmp");

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

	std::vector<Texture> textures5;
	textures5.push_back(Texture());
	textures5[0].id = tex5;
	textures5[0].type = "texture_diffuse";

	std::vector<Texture> textures6;
	textures6.push_back(Texture());
	textures6[0].id = tex6;
	textures6[0].type = "texture_diffuse";

	std::vector<Texture> bodyTextures;
	bodyTextures.push_back(Texture());
	bodyTextures[0].id = bodyTex;
	bodyTextures[0].type = "texture_diffuse";

	std::vector<Texture> mountainTextures;
	mountainTextures.push_back(Texture());
	mountainTextures[0].id = mountainTex;
	mountainTextures[0].type = "texture_diffuse";

	std::vector<Texture> trunkTextures;
	trunkTextures.push_back(Texture());
	trunkTextures[0].id = trunkTex;
	trunkTextures[0].type = "texture_diffuse";

	std::vector<Texture> leavesTextures;
	leavesTextures.push_back(Texture());
	leavesTextures[0].id = leavesTex;
	leavesTextures[0].type = "texture_diffuse";

	Mesh mesh(vert, ind, textures3);

	// Create Obj files - easier :)
	MeshLoaderObj loader;
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
	Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
	Mesh map = loader.loadObj("Resources/Models/plane.obj", textures3);
	Mesh plane = loader.loadObj("Resources/Models/plane.obj", textures6);
	//Mesh player = loader.loadObj("Resources/Models/player.obj", textures2);
	Mesh wallMesh = loader.loadObj("Resources/Models/plane.obj", textures4);
	Mesh terrainMesh = loader.loadObj("Resources/Models/plane.obj", textures5);
	Mesh syringeMesh = loader.loadObj("Resources/Models/syringe.obj", textures4); // Using tower texture as placeholder

	Mesh bodyBox = loader.loadObj("Resources/Models/cube.obj", bodyTextures);

	Mesh trunkBox = loader.loadObj("Resources/Models/cube.obj", trunkTextures);
	Mesh leavesBox = loader.loadObj("Resources/Models/sphere.obj", leavesTextures);

	// Create Terrain
	for (int i = 0; i < 4; i++) {
		const glm::vec3& areaOrigin = terrainOrigins[i];
		for (const auto& cell : layoutTiles) {
			TerrainTile tile;
			tile.position = areaOrigin;
			tile.position.x -= cell.x * stepTilex;
			tile.position.z += cell.y * stepTilez;
			if (cell.x == 0 && !(i&1)) {
				tile.position.x -= cell.x * stepTilex + 5;
				tile.scaleX *= 0.8f;
			}
			if (cell.x == 4 && i&1) {
				tile.scaleX *= 1.2f;
			}
			terrainTiles.push_back({ tile });
		}
	}

	MountainConfig cfg;
	cfg.mapMinX = townMinX;
	cfg.mapMaxX = townMaxX;
	cfg.mapMinZ = townMinZ;
	cfg.mapMaxZ = townMaxZ;
	cfg.baseHeight = BASE_HEIGHT-0.1f;
	cfg.buffer = MOUNTAIN_BUFFER;
	cfg.ramp = MOUNTAIN_RAMP;
	cfg.maxHeight = MAX_MOUNTAIN_HEIGHT;
	cfg.padding = PADDING;
	cfg.gridResolution = 220;
	Mesh mountainMesh = generateMountainMesh(cfg, mountainTextures);

	// Spawn Trees
	spawnTrees(terrainTiles, trees);
	// Big Tree
	Tree bigTree;
	bigTree.position = glm::vec3(-176.0f, -21.5f, 137.0f);
	bigTree.scale = 5.0f;
	bigTree.destructible = true;
	trees.push_back(bigTree);

	// Creates Buildings
	for (int i = 0; i < 4; i++)
		spawnBuilding(origins[i]);

	static double lastX = window.getWidth() / 2.0;
	static double lastY = window.getHeight() / 2.0;

	// Initial mouse position setup to prevent startup jump
	glfwGetCursorPos(window.getWindow(), &lastX, &lastY);

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

		// Left mouse click
		static bool leftClickedLastFrame = false;
		bool leftClickNow = glfwGetMouseButton(window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		bool crosshairClicked = leftClickNow && !leftClickedLastFrame;
		leftClickedLastFrame = leftClickNow;

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;

		// Get ray from camera
		glm::vec3 rayDir = glm::normalize(camera.getCameraViewDirection());
		glm::vec3 rayOrigin = camera.getCameraPosition() + rayDir * 0.6f;

		for (Tree& tree : trees)
		{
			if (!tree.alive || !tree.destructible)
				continue;

			if (tree.hitCooldown > 0.0f)
				tree.hitCooldown -= deltaTime;

			if (crosshairClicked && IsTreeTargeted(tree, rayOrigin, rayDir))
			{
				if (tree.hitCooldown <= 0.0f)
				{
					tree.hitCount++;
					tree.hitCooldown = 0.5f;

					std::cout << "Tree hit: "<< tree.hitCount << "/5" << std::endl;

					if (tree.hitCount >= 5)
					{
						tree.alive = false;
						std::cout << "TREE DESTROYED" << std::endl;
					}
				}
			}
		}

		// GARBAGE ITEM SPAWNING AND COLLECTION
		itemSpawnTimer += deltaTime;
		if (itemSpawnTimer >= ITEM_SPAWN_INTERVAL) {
			itemSpawnTimer = 0.0f;
			for(int i=0; i<5; i++) {
                float randomX = townMinX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (townMaxX - townMinX)));
                float randomZ = townMinZ + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (townMaxZ - townMinZ)));
				garbageItems.push_back({ glm::vec3(randomX, -19.0f, randomZ), 0.0f });
            }
            std::cout << "10 Syringes spawned!" << std::endl;
		}

		for (auto it = garbageItems.begin(); it != garbageItems.end(); ) {
			float dist = glm::distance(playerPos, it->position);
			if (dist < ITEM_COLLECT_RADIUS) {
				playerBackpackCount++;

				playerScore += 150;

				if (playerHP < maxPlayerHP) // Heal player on collection
				{
					playerHP += 5.0f;
					if (playerHP > maxPlayerHP) playerHP = maxPlayerHP; // Cap at 100

					std::cout << "Trash collected! Healed +5 HP. Current: " << (int)playerHP << std::endl;
				}

				if (playerBackpackCount >= MAX_BACKPACK) {
						playerBackpackCount = 0; // Reset Backpack
						playerAntidoteCount++;   // Gain 1 Ammo
						playerScore += 350; // Bonus Score for crafting
						std::cout << ">>> CRAFTED ANTIDOTE! Total: " << playerAntidoteCount << " <<<" << std::endl;
				}
				it = garbageItems.erase(it);
			} // TODO : Edge case : Backpack is Full (5/5) AND we couldn't craft (Ammo Full)
			else {
				it->rotationY += 50.0f * deltaTime;
				++it;
			}
		}

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
		if (!gui.showGUI)
		{
			float xoffset = (xpos - lastX) * mouseSensitivity;
			float yoffset = (lastY - ypos) * mouseSensitivity; // Reversed since y-coordinates range from bottom to top

			camYaw += xoffset;
			camPitch += yoffset;
			camPitch = glm::clamp(camPitch, -60.0f, 10.0f);
		}
		lastX = xpos;
		lastY = ypos;

		ImGuiIO& io = ImGui::GetIO();
		//if (!io.WantCaptureMouse && !gui.showGUI)
		//{
		//camYaw += xoffset;
		//camPitch += yoffset;
		//camPitch = glm::clamp(camPitch, -60.0f, 10.0f);
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

			glm::vec3 nextPos = playerPos;
			nextPos.x += moveDir.x * velocity; // X
			if (!collidesWithBuildings(nextPos) && !collidesWithTrees(nextPos))
				playerPos.x = nextPos.x;

			nextPos = playerPos;
			nextPos.z += moveDir.z * velocity; // Z
			if (!collidesWithBuildings(nextPos) && !collidesWithTrees(nextPos))
				playerPos.z = nextPos.z;

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
			int attempt = 5;

			while (attempt--) {
				float offsetX = ((float)rand() / RAND_MAX - 0.5f) * spawnDistance * 2.0f;
				float offsetZ = ((float)rand() / RAND_MAX - 0.5f) * spawnDistance * 2.0f;

				m.position = glm::vec3(
					glm::clamp(playerPos.x + offsetX, townMinX, townMaxX),
					playerPos.y,
					glm::clamp(playerPos.z + offsetZ, townMinZ, townMaxZ)
				);
				m.bodyScale = 1.0f;
				if (!collidesWithBuildings(m.position))
				{
					monsters.push_back(m);
					break;
				}
			}
		}

		// Ground 
		playerPos.x = glm::clamp(playerPos.x, mapMinX, mapMaxX);
		playerPos.y = BASE_HEIGHT + 0.1f;
		playerPos.z = glm::clamp(playerPos.z, mapMinZ, mapMaxZ);

		/*// Camera orbit
		glm::vec3 offset;
		offset.x = cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
		offset.y = sin(glm::radians(camPitch));
		offset.z = sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));

		glm::vec3 cameraPos = playerPos - offset * cameraDistance;
		camera.setCameraPosition(cameraPos);
		camera.setCameraViewDirection(playerPos - cameraPos);*/

		// First person camera
		glm::vec3 front;
		front.x = cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
		front.y = sin(glm::radians(camPitch));
		front.z = sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));
		front = glm::normalize(front);

		glm::vec3 eyeOffset = glm::vec3(0.0f, 1.4f, 0.0f);
		camera.setCameraPosition(playerPos + eyeOffset);
		camera.setCameraViewDirection(front);

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

		/*box.draw(shader);
		totalRenderedObjects++;*/

		// Draw mountains
		ModelMatrix = glm::mat4(1.0f);
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		mountainMesh.draw(shader);
		totalRenderedObjects++;

		// Draw Map
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
				map.draw(shader);
				totalRenderedObjects++;
			}
		}

		for (int x = -1; x <= 2; x++)
		{
			for (int z = -1; z <= 2; z++)
			{
				if (x >= 0 && x < 2 && z >= 0 && z < 2)
					continue;

				ModelMatrix = glm::mat4(1.0f);
				ModelMatrix = glm::translate(
					ModelMatrix,
					glm::vec3(baseX + x * stepX, groundY + 0.25f, baseZ + z * stepZ)
				);
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				plane.draw(shader); 
				totalRenderedObjects++;
			}
		}

		for (const TerrainTile& terrainTile : terrainTiles) {
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, terrainTile.position);
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(terrainTile.scaleX, 0.1f, terrainTile.scaleZ));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			terrainMesh.draw(shader);
			totalRenderedObjects++;
		}

		for (const Building& building : buildings) {
			for (const Wall& wall : building.walls) {
				ModelMatrix = glm::mat4(1.0f);
				//ModelMatrix = glm::translate(ModelMatrix, building.position);
				ModelMatrix = glm::translate(ModelMatrix, wall.localPos);
				ModelMatrix = glm::rotate(ModelMatrix, (90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				if (wall.rotateY) {
					ModelMatrix = glm::rotate(ModelMatrix, (90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				}
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.153f, 0.1f, 0.153f));

				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

				wallMesh.draw(shader);
				totalRenderedObjects++;
			}
		}

		for (const Tree& tree : trees) {
			if (!tree.alive)
				continue;
			glm::mat4 treeModel = glm::mat4(1.0f);
			treeModel = glm::translate(treeModel, tree.position);
			treeModel = glm::scale(treeModel, glm::vec3(tree.scale));
			// n determines complexity
			drawtree3D(shader, trunkBox, leavesBox, ProjectionMatrix, ViewMatrix,
					treeModel, 4, 5, 0.6f, 0.2f);
		}

		/*/// SIMPLE HUMAN MODEL ///

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
		}*/

		bodyShader.use();
		glUniform3f(glGetUniformLocation(bodyShader.getId(), "bodyColor"), 0.0f, 1.0f, 0.0f);


		for (size_t i = 0; i < monsters.size(); i++)
		{
			Monster& m = monsters[i];
			glm::vec3 dir = playerPos - m.position;
			float distance = glm::length(dir);
			float distanceToPlayer = glm::distance(playerPos, m.position);

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
				float moveStep = monsterSpeed * deltaTime;

				glm::vec3 nextPos = m.position; // X
				nextPos.x += dir.x * moveStep;
				if (!collidesWithBuildings(nextPos) && !collidesWithTrees(nextPos))
					m.position.x = nextPos.x;

				nextPos = m.position;
				nextPos.z += dir.z * moveStep; // Z
				if (!collidesWithBuildings(nextPos) && !collidesWithTrees(nextPos))
					m.position.z = nextPos.z;

				m.yaw = glm::degrees(atan2(dir.z, dir.x)) + 90.0f;
				m.walkCycle += moveStep * walkSpeedFactor;
			}

			// Crosshair kill logic
			if (crosshairClicked && IsMonsterTargeted(m, rayOrigin, rayDir, 1.0f) && distanceToPlayer <= maxKillDistance ) // 1.0f is radius
			{
				std::cout << ">>> MONSTER KILLED! <<<" << std::endl;
				playerScore += 500;
				monsters.erase(monsters.begin() + i);
				i--;
				continue;
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
			playerScore = 0; // Reset Score
			playerBackpackCount = 0; // Clear Backpack
			playerPos = glm::vec3(1.0f, -19.0f, 1.0f); // Reset Position
			monsters.clear(); // Clear enemies
		}

		// 3. Debug Print (Every 1 sec approx, or use ImGui text if available)
		// We will just print if damaged to avoid spam
		if (damageFlashTimer > 0.15f) { // Only print on initial hit frame approx
			std::cout << "HP: " << (int)playerHP << " / " << (int)maxPlayerHP << std::endl;
		}
		// -------------------------------


		// --- RENDER SYRINGES ---
		shader.use();
		for (const auto& s : garbageItems) {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, s.position);
			model = glm::rotate(model, glm::radians(s.rotationY), glm::vec3(0, 1, 0));
			model = glm::scale(model, glm::vec3(0.1f));

			// Assuming ProjectionMatrix, ViewMatrix, MatrixID2, ModelMatrixID are available in this scope
			glm::mat4 mvp = ProjectionMatrix * ViewMatrix * model;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &mvp[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

			syringeMesh.draw(shader); // TODO : Replace with syringe mesh
			totalRenderedObjects++;
		}

		gui.Render(playerPos, window.getWidth(), window.getHeight(), displayFPS, totalRenderedObjects, playerScore, (int)playerHP, playerBackpackCount, playerAntidoteCount);

		// TEMPORARY DEBUG KEY
		static bool fPressed = false;
		if (window.isPressed(GLFW_KEY_F)) {
			if (!fPressed) {
				gui.questManager.CompleteCurrentQuest();
				gui.AddLog("[DEBUG] Force Completed Quest"); // Log it so you know
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
				if (gui.showGUI)
				{
					// MENU OPENED: Show Cursor let it move freely
					glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

					// Reset ImGui mouse states so it doesn't think you are dragging
					ImGuiIO& io = ImGui::GetIO();
					io.MouseDown[0] = false;
				}
				else
				{
					// MENU CLOSED: Hide Cursor lock it to center
					glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

					// Reset the "last" mouse position to the current position
					glfwGetCursorPos(window.getWindow(), &lastX, &lastY);
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

void spawnBuilding(const glm::vec3& origin)
{
	for (const auto& cell : layoutBuildings)
	{
		Building building;
		for (const Wall& w : baseWalls)
		{
			glm::vec3 pos = w.localPos;
			pos.x -= origin.x - cell.x * step;
			pos.z += origin.z + cell.y * step;
			building.walls.push_back({ pos, w.rotateY });
		}

		building.minX = 1e9f; 
		building.maxX = -1e9f;
		building.minZ = 1e9f; 
		building.maxZ = -1e9f;

		for (const Wall& w : building.walls)
		{
			building.minX = std::min(building.minX, w.localPos.x);
			building.maxX = std::max(building.maxX, w.localPos.x);
			building.minZ = std::min(building.minZ, w.localPos.z);
			building.maxZ = std::max(building.maxZ, w.localPos.z);
		}

		const float bound = 0.75f;
		building.minX -= bound; 
		building.maxX += bound;
		building.minZ -= bound; 
		building.maxZ += bound;

		buildings.push_back(building);
	}
}

bool collidesWithBuildings(const glm::vec3& p)
{
	for (const Building& b : buildings)
		if (p.x >= b.minX && p.x <= b.maxX &&
			p.z >= b.minZ && p.z <= b.maxZ)
			return true;
	return false;
}

void spawnTrees(const std::vector<TerrainTile>& terrainTiles,std::vector<Tree>& trees)
{
	trees.clear();
	int tileCounter = 0;
	for (const TerrainTile& tile : terrainTiles)
	{
		tileCounter++;
		if (tileCounter % 3 != 0)
			continue;
		if (rand() % 2 == 0)
			continue;
	
		float offsetX = ((float)rand() / RAND_MAX - 0.5f) * stepTilex;
		float offsetZ = ((float)rand() / RAND_MAX - 0.5f) * stepTilez;

		float roadHalfWidth = 5.0f;
		if (fabs(offsetX) < roadHalfWidth || fabs(offsetZ) < roadHalfWidth)
			continue;

		Tree t;
		t.position.x = tile.position.x + offsetX;
		t.position.z = tile.position.z + offsetZ;
		t.position.y = BASE_HEIGHT - 1.5f;
		t.scale = 0.8f + ((float)rand() / RAND_MAX) * 0.6f;

		trees.push_back(t);
	}
}


void drawtree3D(Shader& shader, Mesh& trunkMesh, Mesh& leavesMesh, const glm::mat4& projection, const glm::mat4& view,
			glm::mat4 model, int depth, int segments, float segmentLength, float radius)
{
	GLuint mvpLoc = glGetUniformLocation(shader.getId(), "MVP");
	GLuint modelLoc = glGetUniformLocation(shader.getId(), "model");

	// end case
	if (depth <= 1 || segments <= 1 || radius < 0.02f)
	{
		for (int i = 0; i < segments; i++)
		{
			glm::mat4 leaf = model;
			leaf = glm::translate(leaf, glm::vec3(0.0f, segmentLength * 0.5f, 0.0f));
			leaf = glm::scale(leaf, glm::vec3(radius, segmentLength/5, radius));
			glm::mat4 mvp = projection * view * leaf;
			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &leaf[0][0]);

			leavesMesh.draw(shader);

			model = glm::translate(model, glm::vec3(0.0f, segmentLength, 0.0f));
		}
		return;
	}

	// draw trunk segments
	for (int i = 0; i < segments; i++)
	{
		glm::mat4 segment = model;
		segment = glm::translate(segment, glm::vec3(0.0f, segmentLength * 0.5f, 0.0f));
		segment = glm::scale(segment, glm::vec3(radius, segmentLength, radius));
		glm::mat4 mvp = projection * view * segment;
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &segment[0][0]);

		trunkMesh.draw(shader);

		model = glm::translate(model, glm::vec3(0.0f, segmentLength, 0.0f));
	}

	// branch
	int branchCount = glm::clamp(depth + 1, 2, 5);
	float angleStep = 360.0f / branchCount;
	float tiltAngle = 25.0f + depth * 5.0f;

	for (int i = 0; i < branchCount; i++)
	{
		glm::mat4 b = model;

		b = glm::rotate(b, (i * angleStep), glm::vec3(0, 1, 0));
		b = glm::rotate(b, (tiltAngle), glm::vec3(1, 0, 0));

		drawtree3D(shader, trunkMesh, leavesMesh, projection, view,
				b, depth - 1, segments - 1, segmentLength * 0.9f, radius * 0.7f);
	}
}

bool collidesWithTrees(const glm::vec3& p)
{
	for (const Tree& tree : trees)
	{
		if (!tree.alive)
			continue;

		float radius = 1.6f * tree.scale;
		float dist = glm::length(p - tree.position);

		if (tree.destructible) {
			radius = tree.scale * 1.1;
			dist = glm::length(p - tree.position);
		}
		
		if (dist < radius)
			return true;
	}
	return false;
}


void processKeyboardInput()
{
}