#pragma clang diagnostic warning "-Wall"
#pragma clang diagnostic warning "-Wextra"
#pragma clang diagnostic ignored "-Wformat-security"

#if !defined(VENOM_RELEASE) && !defined(VENOM_STRICT)
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

/*
	TODO(Torin)
	- Camera fusturm culling of the terrain
	- Move the terrain generation system to a multicore generator
	- Make the terrain system use SIMD perlin noise
*/

#include "platform.h"
#include "venom_module.h"

#include "game.h"

#include "opengl_render.h"

#ifdef VENOM_HOTLOAD
#ifndef VENOM_RELEASE
#include "opengl_debug.h"
#include "engine_debug.cpp"
#include "opengl_resources.cpp"
#endif//VENOM_RELEASE
#endif//VENOM_HOTLOAD

#include "venom_noise.cpp"
#include "math_procedural.cpp"
#include "terrain.cpp"
#include "opengl_glsl.cpp"


//TODO(Torin) Possibly change this function to a aquire memory function?
//That way if we go down the path of allowing mod support all of the avaiable modules
//could have their aquire_memory functions called and then after all the memory has been
//delegated the asset cache can be initalized and then a GameLoad function can be called

extern "C" void GameStartup(GameMemory *memory)
{
	RenderState *rs = &memory->renderState;
	GameAssets *assets = &memory->assets;
	SystemInfo *sys = &memory->systemInfo;	
	imgui_init(memory);

	U8* pixels;
	int width, height;
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	memory->renderState.imguiFontTexture = textureID;
	io.Fonts->TexID = (void *)(size_t)memory->renderState.imguiFontTexture;

	float far_plane_distance = (TERRAIN_CHUNK_PER_EDGE*0.5)* TERRAIN_CHUNK_SIZE;
	InitializeCamera(&rs->camera, 45.0f * DEG2RAD, 0.1f, far_plane_distance);
	InitializeCamera(&rs->debugCamera, 45.0f * DEG2RAD, 0.1f, far_plane_distance);

	rs->camera.position = V3(0.0f, 1.0f, 0.0f);
	rs->debugCamera.position = V3(75.0f, 30.0f, -110.0f);
	rs->debugCamera.front = V3(-0.7f, 0.0f, 0.7f);

	//Initalize RenderState
	InitSubBlock("vertex_block", &rs->vertexBlock, MEGABYTES(128), &memory->mainBlock);
	InitSubBlock("intex_block", &rs->indexBlock, MEGABYTES(128), &memory->mainBlock);

	GameEntities *entities = &memory->entities;
	entities->player.pos = V3(TERRAIN_ORIGIN_TO_CENTER_CHUNK_OFFSET*TERRAIN_CHUNK_SIZE, 0.001,
		TERRAIN_ORIGIN_TO_CENTER_CHUNK_OFFSET*TERRAIN_CHUNK_SIZE);

	{ //@Terrain @Init
		TerrainGenerationState *terrainGenState = &memory->terrainGenState;
#if 0
		static const U32 maxTerrainVertexCount = TERRAIN_VERTEX_COUNT_PER_CHUNK * TERRAIN_TOTAL_CHUNK_COUNT;
		static const U32 maxTerrainIndexCount = TERRAIN_INDEX_COUNT_PER_CHUNK * TERRAIN_TOTAL_CHUNK_COUNT;
		size_t terrain_detail_map_memory_requirement = TERRAIN_VERTEX_COUNT_PER_CHUNK * TERRAIN_TOTAL_CHUNK_COUNT;
		size_t requiredTerrainMemory =  (sizeof(Vertex3D) * maxTerrainVertexCount) +
										(sizeof(U32) * maxTerrainIndexCount) +
										(sizeof(TerrainEntity) * TERRAIN_ENTITY_MAX) +
										(terrain_detail_map_memory_requirement);


		InitSubBlock("TerrainMemory", &terrainGenState->memory, requiredTerrainMemory, &memory->mainBlock);
		terrainGenState->verticesBase = ReserveArray(Vertex3D, maxTerrainVertexCount, &terrainGenState->memory);
		terrainGenState->indicesBase  = ReserveArray(U32, maxTerrainIndexCount, &terrainGenState->memory);
		terrainGenState->entityBase   = ReserveArray(TerrainEntity, TERRAIN_ENTITY_MAX, &terrainGenState->memory);
		terrainGenState->detail_map_base = PushSize(terrain_detail_map_memory_requirement, &terrainGenState->memory);

		CreateIndexedVertexArray3D(&terrainGenState->vertexArray,
			maxTerrainVertexCount, maxTerrainIndexCount, nullptr, nullptr, GL_DYNAMIC_DRAW); 
#else
		size_t requiredTerrainMemory = (TERRAIN_TOTAL_VERTEX_COUNT) + //heightmap
									   (TERRAIN_TOTAL_VERTEX_COUNT) + //detailmap
									   ((TERRAIN_TOTAL_VERTEX_COUNT) * sizeof(V3)) + //normals
									   (TERRAIN_ENTITIES_PER_CHUNK * TERRAIN_TOTAL_CHUNK_COUNT * sizeof(TerrainEntity));

		InitSubBlock("TerrainMemory", &terrainGenState->memory, requiredTerrainMemory, &memory->mainBlock);
		terrainGenState->heightmap_base = ReserveArray(U8, TERRAIN_TOTAL_VERTEX_COUNT, &terrainGenState->memory);
		terrainGenState->detailmap_base = ReserveArray(U8, TERRAIN_TOTAL_VERTEX_COUNT, &terrainGenState->memory);
		terrainGenState->normals_base   = ReserveArray(V3, TERRAIN_TOTAL_VERTEX_COUNT, &terrainGenState->memory); 
		terrainGenState->entityBase 	= ReserveArray(TerrainEntity, TERRAIN_ENTITIES_PER_CHUNK * TERRAIN_TOTAL_CHUNK_COUNT, &terrainGenState->memory);

		terrainGenState->heightmap_texture_array = CreateTextureArray(
				TERRAIN_CELLS_PER_EDGE + 1, TERRAIN_CELLS_PER_EDGE + 1, TERRAIN_TOTAL_CHUNK_COUNT, 
				GL_R8, GL_REPEAT, GL_LINEAR); 
		terrainGenState->detailmap_texture_array = CreateTextureArray(
				TERRAIN_CELLS_PER_EDGE + 1, TERRAIN_CELLS_PER_EDGE + 1, TERRAIN_TOTAL_CHUNK_COUNT, 
				GL_R8, GL_REPEAT, GL_LINEAR); 
		terrainGenState->normals_texture_array = CreateTextureArray(
				TERRAIN_CELLS_PER_EDGE + 1, TERRAIN_CELLS_PER_EDGE + 1, TERRAIN_TOTAL_CHUNK_COUNT, 
				GL_RGB16F, GL_REPEAT, GL_LINEAR);

		//TODO(Torin) Creating the terrain base mesh should probably be moved out somewhere else
		//beacause in release mode it will just be baked into the executable as static data
		//so having the code here is not representative of the actualy runtime characteristics of the application

		V2* vertices = terrainGenState->vertices;
		U32* indices = terrainGenState->indices;

		U32 currentVertexIndex = 0;
		for (U32 z = 0; z < TERRAIN_CELLS_PER_EDGE + 1; z++) 
		{
			for (U32 x = 0; x < TERRAIN_CELLS_PER_EDGE + 1; x++) 
			{
				V2 *vertex = &vertices[currentVertexIndex];
				vertex->x =  ((float)x * (float)TERRAIN_CELL_SIZE);
				vertex->y =  ((float)z * (float)TERRAIN_CELL_SIZE);
				currentVertexIndex++;
			}
		}


		U32 currentIndex = 0;
		for (U32 z = 0; z < TERRAIN_CELLS_PER_EDGE; z++) 
		{
			for (U32 x = 0; x < TERRAIN_CELLS_PER_EDGE; x++) 
			{
				indices[currentIndex++] = ((z + 0) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 0);
				indices[currentIndex++] = ((z + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 0); 
				indices[currentIndex++] = ((z + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 1);
				indices[currentIndex++] = ((z + 0) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 0);
				indices[currentIndex++] = ((z + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 1);
				indices[currentIndex++] = ((z + 0) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 1);
			}
		}

		for(U32 i = 0; i < TERRAIN_TOTAL_CHUNK_COUNT; i++)
		{
			terrainGenState->instanceModelMatrices[i] = M4Identity();
		}


		glGenVertexArrays(1, &terrainGenState->base_mesh.vertexArrayID);
		glBindVertexArray(terrainGenState->base_mesh.vertexArrayID);
		
		glGenBuffers(1, &terrainGenState->base_mesh.vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->base_mesh.vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, 
				TERRAIN_VERTEX_COUNT_PER_CHUNK * sizeof(V2),	vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &terrainGenState->base_mesh.indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainGenState->base_mesh.indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
				TERRAIN_INDEX_COUNT_PER_CHUNK * sizeof(U32), indices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V2), (GLvoid*)0);

		glGenBuffers(1, &terrainGenState->instanceBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->instanceBufferID);
		glBufferData(GL_ARRAY_BUFFER, TERRAIN_TOTAL_CHUNK_COUNT * sizeof(M4), 
				terrainGenState->instanceModelMatrices, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 0));
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 1));
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 2));
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 3));
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glBindVertexArray(0);
	
#endif	
		terrainGenState->lastGenerationTriggerX = entities->player.pos.x;
		terrainGenState->lastGenerationTriggerZ = entities->player.pos.z;

		//TODO(Torin) Add terrain generation parameters and different biomes for the 
		//terrain generation system.  Perhaps we can do an inhabited style solar system
		TerrainGenerationParameters *terrainGenParams = &memory->terrainGenParams;
		terrainGenParams->seed = 0;
		//TODO(Torin) Make the terrain chunks consider their neighors when calculating
		//the surface normals for each of the edge vertices
	} //Terrain Init

	{ //@Skydome Init
		static const int skydomeResolution = 8;
		MeshData skydomeMeshData;
		GetSubdiviedCubeVertexAndIndexCount(skydomeResolution, &skydomeMeshData.vertexCount, &skydomeMeshData.indexCount);
		skydomeMeshData.vertices = ReserveArray(Vertex3D, skydomeMeshData.vertexCount, &memory->mainBlock);
		skydomeMeshData.indices = ReserveArray(U32, skydomeMeshData.indexCount, &memory->mainBlock);
		GenerateSubdiviedCubeMeshData(&skydomeMeshData,skydomeResolution);
		for (U32 i = 0; i < skydomeMeshData.vertexCount; i++) {
			skydomeMeshData.vertices[i].position = Normalize(skydomeMeshData.vertices[i].position);
		}
		CreateIndexedVertexArray3D(&memory->skydomeVertexArray, &skydomeMeshData);
		memory->skydomeIndexCount = skydomeMeshData.indexCount;
	}

	{ //@Shadow @CSM
		glGenFramebuffers(1, &rs->depth_map_framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, rs->depth_map_framebuffer);
		glDrawBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glGenTextures(1, &rs->depth_map_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, rs->depth_map_texture);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
			SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, 
			SHADOW_MAP_CASCADE_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			rs->csm_cascade_frustums[i].aspect_ratio = sys->screen_width / sys->screen_height;
			rs->csm_cascade_frustums[i].field_of_view = rs->camera.fov + 0.2f;
		}
	}
	
	{ //@Graphics 
		rs->imguiRenderGroup = CreateImGuiRenderGroup();
		rs->solidDebugGroup = CreateDebugRenderGroup();
		rs->lineDebugGroup = CreateDebugRenderGroup();

		rs->debugShader = GetShaderProgram(assets, ShaderID_Debug);
		rs->singleColorShader = GetShaderProgram(assets, ShaderID_SingleColor);
		rs->imguiRenderGroupShader = GetShaderProgram(assets, ShaderID_Sprite);
		rs->debugNormalsShader = GetShaderProgram(assets, ShaderID_debug_normals);
		
		rs->terrain_shader = GetShaderProgram(assets, ShaderID_terrain);
		rs->skydome_shader = GetShaderProgram(assets, ShaderID_skydome);
		rs->depth_map_shader = GetShaderProgram(assets, ShaderID_depth_map);
		rs->material_opaque_shader = GetShaderProgram(assets, ShaderID_material_opaque);
		rs->material_transparent_shader = GetShaderProgram(assets, ShaderID_material_transparent);

		rs->debug_depth_map_shader = GetShaderProgram(assets, ShaderID_debug_depth_map);

		const Camera& camera = rs->camera;

		rs->lightingState.directionalLights[0].direction = V3(0.2f, 0.9f, 0.0f);
		rs->lightingState.directionalLights[0].color = V3(0.9f, 0.9f, 0.9f);
		rs->lightingState.directionalLightCount = 1;
	}

	//Initalize Game
	
#if 0
	RNGSeed seed(0);
	InitalizeEntityArray(ParticleSystem, 64, &entities->ParticleSystemArray, &memory->mainBlock);
	InitalizeEntityArray(DebugEntity, 10000, &entities->DebugEntityArray, &memory->mainBlock);

	const V3 spawn_min = V3(0.0f, 0.0f, 0.0f);
	const V3 spawn_max = V3(100.0f, 100.0f, 100.0f);
	const V3 bounds_min = V3(0.2);
	const V3 bounds_max = V3(3.0);
	for (size_t i = 0; i < 10000; i++)
	{
		DebugEntity *entity = CreateEntity(DebugEntity, &entities->DebugEntityArray);
		entity->position = RandomInRange(seed, spawn_min, spawn_max);
		entity->color = RandomSolidColor(seed);
		entity->size = { RandomInRange(seed, bounds_min, bounds_max), 
			RandomInRange(seed, bounds_min, bounds_max) };
	}
#endif
	
	//TODO(Torin) This should NOT happen in the game code!
	assert(memory->mainBlock.size > memory->mainBlock.used);
	U64 remainingBlockMemory = memory->mainBlock.size - memory->mainBlock.used;
	InitSubBlock("AssetCache", &memory->assets.memory, remainingBlockMemory, &memory->mainBlock);
}


void GameLoad(GameMemory *memory)
{	
	GetModelDrawable(&memory->assets, DEBUGModelID_player); 
	TerrainGenerationState *terrainGenState = &memory->terrainGenState;
	for (U32 z = 0; z < TERRAIN_CHUNK_PER_EDGE; z++) 
		for (U32 x = 0; x < TERRAIN_CHUNK_PER_EDGE; x++) 
			GenerateTerrainChunk(terrainGenState, &memory->assets, x, z);
}

static inline void GameTriggeredBreakpoint(InputState *input)
{
	if (input->isKeyDown[KEYCODE_BACKSPACE])
		input->isKeyDown[KEYCODE_BACKSPACE] = 0;
}

static void ShowMemoryBlockTree(MemoryBlock *block, U32 index = 0)
{
	//NOTE(Torin) This should break with 2 levels of tree depth
	//but there is no reason the block hiarchy should ever be that way 
	float blockUsedPercentage = (int)(((float)block->used / (float)block->size) * 100);
	float blockSizeInMegabytes = (float)block->size / (float)MEGABYTES(1);
	float blockUsedInMegabytes = (float)block->used / (float)MEGABYTES(1);
	if ((ImGui::TreeNode((void*)index, "%s : (%.2f MB / %.2f MB) : %.1f%%", 
		block->name, blockUsedInMegabytes, blockSizeInMegabytes, blockUsedPercentage)))
	{
		for (U32 i = 0; i < block->childCount; i++)
		{
			ShowMemoryBlockTree(block->children[i], index + i);
		}
		ImGui::TreePop();
	}
}

#if 0
	auto GetMouseRayDirection = [&](const Camera& camera) -> V3
{
		float ndcX = (2.0f * ((float)input->cursorPosX / system->screen_width)) - 1.0f;
		float ndcY = 1.0f - (2.0f * (input->cursorPosY / system->screen_height));
		auto clip_space = V4(ndcX, ndcY, -1.0f, 1.0f);
		auto eye_space = Inverse(camera.projection) * clip_space;
		eye_space = V4(eye_space.x, eye_space.y, -1.0f, 0.0f);
		V4 world_space = Inverse(camera.view) * eye_space;
		V3 result = V3(world_space.x, world_space.y, world_space.z);
		result = Normalize(result);
		return result;
	};

	auto TestRayPlaneIntersection = [](const V3& rayOrigin, const V3& rayDirection, const V3& planeNormal, const V3& planeCenter) -> bool
	{
		auto t = Dot((planeCenter - rayOrigin), planeNormal) / Dot(planeNormal, rayDirection);
		bool result = t > 0;
		return result;
	};
#endif




//TODO(Torin) Add collisions with the static terrain entities
//TODO(Torin) Pull out the player specific code to a SimulateDynamicBody
//function
static void SimulatePhysics(GameMemory *memory)
{
	static const float PHYSICS_TIME_STEP = 1.0f / 60.0f;
	static const float PHYSICS_GRAVITY_ACCELERATION = 9.81f;

	Player &player = memory->entities.player;
	TerrainGenerationState *terrainGenState = &memory->terrainGenState;
	player.vel.y -= PHYSICS_GRAVITY_ACCELERATION * PHYSICS_TIME_STEP;
	player.pos += player.vel * PHYSICS_TIME_STEP;
	float ground_height = GetTerrainHeightAtWorldPosition(terrainGenState, player.pos.x, player.pos.z);
	if (player.pos.y > ground_height)
	{
		float deltaY = player.vel.y * PHYSICS_TIME_STEP;
		if (player.pos.y + deltaY > ground_height)
		{
			player.is_grounded = false;
			player.pos.y += deltaY;
		}
		else
		{
			player.pos.y = ground_height;
			player.vel.y = 0;
		}
	}
	else
	{
		player.is_grounded = true;
		player.pos.y = ground_height;
		player.vel.y = 0.0f;
	}
}

extern "C" void GameUpdate(GameMemory *memory)
{
	static bool is_loaded = false;
	if (!is_loaded)
	{
		GameLoad(memory);
		is_loaded = true;
	}


	GameState *gameState = &memory->gameState;
	DebugGameplaySettings *gameplay_settings = &gameState->debug_settings;
	InputState *input = &memory->inputState;
	RenderState *rs = &memory->renderState;
	GameEntities *entities = &memory->entities;

	imgui_update_state(memory);
	ImGui::NewFrame();

	if (input->isKeyDown[KEYCODE_CTRL] && input->isKeyDown[KEYCODE_Q])
	   gameState->isRunning = false;
	
	if (input->toggleDebugModePressed)
	{
		gameplay_settings->use_debug_camera = !gameplay_settings->use_debug_camera;
		rs->debugCamera.position = rs->camera.position;
		rs->debugCamera.yaw = rs->camera.yaw;
		rs->debugCamera.pitch = rs->camera.pitch;
	}

	SystemInfo *system = &memory->systemInfo;
	Camera *camera = gameplay_settings->use_debug_camera ? &rs->debugCamera : &rs->camera;

	UpdateCamera(camera);
	camera->view = LookAt(camera->position, camera->front + camera->position, V3(0.0f, 1.0f, 0.0f));
	camera->projection = Perspective(camera->fov, system->screen_width, system->screen_height,
		camera->near_clip, camera->far_clip);


	if (!gameplay_settings->use_debug_camera)
	{ //@Player @Controls
		Player &player = entities->player;
		player.vel.x *= 0.9f;
		player.vel.z *= 0.9f;
		V3 dv;

		static const float ACCELERATION = 1.0f;
		if (input->isKeyDown[KEYCODE_W])
		{
			dv.x += ACCELERATION * cosf(player.angle);
			dv.z += ACCELERATION * sinf(player.angle);
		}
		if (input->isKeyDown[KEYCODE_A])
		{
			dv.x += ACCELERATION * sinf(player.angle);
			dv.z -= ACCELERATION * cosf(player.angle);
		}
		if (input->isKeyDown[KEYCODE_S])
		{
			dv.x -= ACCELERATION * cosf(player.angle);
			dv.z -= ACCELERATION * sinf(player.angle);
		}
		if (input->isKeyDown[KEYCODE_D])
		{
			dv.x -= ACCELERATION * sinf(player.angle);
			dv.z += ACCELERATION * cosf(player.angle);
		}

		float maxSpeed = 4.0f;
		if (input->isKeyDown[KEYCODE_SHIFT])
		{
			maxSpeed = 10.0f;
		}

		V2 lateralVelocity = V2(dv.x, dv.z);
		float magnitudeSquared = MagnitudeSquared(lateralVelocity);
		if (magnitudeSquared > 0.01f)
		{
			float magnitude = sqrtf(magnitudeSquared);
			if (magnitude > maxSpeed)
			{
				lateralVelocity = Normalize(lateralVelocity);
				lateralVelocity *= maxSpeed;
				dv.x = lateralVelocity.x;
				dv.z = lateralVelocity.y;
			}
		}
		
		float absoluteMagnitude = Magnitude(dv);
		if (absoluteMagnitude > maxSpeed)
		{
			dv = Normalize(dv);
			dv *= maxSpeed;
		}

		if (input->isKeyDown[KEYCODE_SPACE] && player.is_grounded)
		{
			dv.y += 10.0f;
		}

		player.vel += dv;

		camera->yaw += (PI32 / 180.0f) * input->cursorDeltaX;
		camera->pitch += (PI32 / 180.0f) * input->cursorDeltaY;
		camera->pitch = Clamp(camera->pitch, DEG2RAD*-80.0f, DEG2RAD*20.0f);

		const static float CAMERA_DIST = 12.0f;
		//V3 cameraView = V3(cos(camera->yaw)*cos(camera->pitch), sin(camera->pitch), -sin(camera->yaw)*cos(camera->pitch));
		//V3 cameraRight = Cross(cameraView, V3(0.0f, 1.0f, 0.0f));
		//V3 cameraLoc = -cameraRight * cameraView;

		//camera->position = cameraLoc;

		player.angle = camera->yaw;

		camera->position = player.pos + V3(
			-CAMERA_DIST * cosf(camera->yaw) * cosf(camera->pitch),
			-sin(camera->pitch) * CAMERA_DIST,
			-CAMERA_DIST * sinf(camera->yaw)*cosf(camera->pitch));
		camera->position.y += 3.0f;
	}

	else if (input->isButtonDown[MOUSE_RIGHT])
	{
		MoveCameraWithFPSControls(memory, camera);
	}

#define ImGuiTextV3(v) ImGui::Text("%s: [%f, %f, %f]", #v, v.x, v.y, v.z)
#define ImGuiBoolEdit(b) ImGui::Checkbox(#b, &b)
#define ImGuiIntSlideEdit(i, min, max) ImGui::SliderInt(#i, &i, min, max);

#if 1
	ImGui::Begin("InputState");
	ImGui::Text("CursorX: %d", input->cursorPosX);
	ImGui::Text("CursorY: %d", input->cursorPosY);
	ImGui::Text("CursorDeltaX: %d", input->cursorDeltaX);
	ImGui::Text("CursorDeltaY: %d", input->cursorDeltaY);
	ImGui::Text("LeftButtonDown: %s", input->isButtonDown[MOUSE_LEFT] ? "TRUE" : "FALSE");
	ImGui::Text("RightButtonDown: %s", input->isButtonDown[MOUSE_RIGHT] ? "TRUE" : "FALSE");

	ImGui::Begin("DebugGameplaySettings");
	ImGuiBoolEdit(gameplay_settings->disable_terrain_generation);
	ImGuiBoolEdit(gameplay_settings->use_debug_camera);
	ImGui::End();

#if 0
	for(auto i = 0; i < input->keysPressedCount; i++)
	{
		ImGui::Text("KeysDown: %d", input->keycodes[i]);
	}
#endif
	ImGui::End();
#endif
	
	double ram_in_gb = (double)system->virtual_memory_size/ (1 << 30);
	ImGui::Begin("SystemInfo");
	ImGui::Text("SystemPhysicalMemory: %f", ram_in_gb);
	ImGui::End();



	ImGui::Begin("CameraInfo");
	ImGuiTextV3(camera->position);
	ImGuiTextV3(camera->front);

	ImGui::Text("Position: [%f, %f, %f]", camera->position.x, camera->position.y, camera->position.z);
	ImGui::Text("Pitch: %f", RAD2DEG*camera->pitch);
	ImGui::Text("Yaw: %f", RAD2DEG*camera->yaw);

	ImGui::End();

	ImGui::Begin("RenderSettings");
	ImGui::Checkbox("WireframeEnabled", &rs->debugSettings.is_wireframe_enabled);
	ImGui::Checkbox("DebugNormals", &rs->debugSettings.render_debug_normals);
	ImGui::Checkbox("render_from_directional_light", &rs->debugSettings.render_from_directional_light);
	ImGuiBoolEdit(rs->debugSettings.draw_shadowmap_depth_texture);
	ImGuiIntSlideEdit(rs->debugSettings.draw_shadow_map_index, 0, SHADOW_MAP_CASCADE_COUNT);

	ImGui::SliderFloat3("DirectionalLight", &rs->lightingState.directionalLights[0].direction.x, -1.0f, 1.0f);
	ImGui::End();

	EntityArray *debugEntities = &entities->DebugEntityArray;
#define ITER_ENTITIES(T, E, EA) for (T *E = (T*) EA->entities; (uintptr_t)E < (uintptr_t)(((T*) EA->entities) + EA->count); E++)

#if 0
	ITER_ENTITIES(DebugEntity, e, debugEntities)
	{
		PushBox(&rs->solidDebugGroup, e->position, e->size.min, e->size.max, e->color);
	}
#endif


#if 1


	{ //@Terrain Generation
		TerrainGenerationState *terrainGenState = &memory->terrainGenState;
		V3 view_position = gameplay_settings->use_debug_camera ? camera->position : entities->player.pos;
		float offsetFromCurrentChunkX = view_position.x - terrainGenState->lastGenerationTriggerX;
		float offsetFromCurrentChunkZ = view_position.z - terrainGenState->lastGenerationTriggerZ;
		
		if (!gameplay_settings->disable_terrain_generation)
		{
			if (offsetFromCurrentChunkX > TERRAIN_CHUNK_SIZE || offsetFromCurrentChunkX < (-(S32)TERRAIN_CHUNK_SIZE))
			{
				terrainGenState->gpuMemoryOriginX += (offsetFromCurrentChunkX > 0.0f ? 1 : -1);
				if (terrainGenState->gpuMemoryOriginX >= (S32)TERRAIN_CHUNK_PER_EDGE) terrainGenState->gpuMemoryOriginX = 0;
				if (terrainGenState->gpuMemoryOriginX < 0) terrainGenState->gpuMemoryOriginX = TERRAIN_CHUNK_PER_EDGE - 1;
				terrainGenState->lastGenerationTriggerX = view_position.x;
				
				U32 generationChunkIndexX = offsetFromCurrentChunkX > 0 ? (TERRAIN_CHUNK_PER_EDGE - 1) : 0;
				terrainGenState->currentOriginX += offsetFromCurrentChunkX > 0 ? 1 : -1;
				for (U32 i = 0; i < TERRAIN_CHUNK_PER_EDGE; i++) {
					GenerateTerrainChunk(terrainGenState, &memory->assets, generationChunkIndexX, i);
				}	
			}

			if (offsetFromCurrentChunkZ > TERRAIN_CHUNK_SIZE || offsetFromCurrentChunkZ < -(S32)TERRAIN_CHUNK_SIZE)
			{
				terrainGenState->gpuMemoryOriginZ += (offsetFromCurrentChunkZ > 0.0f ? 1 : -1);
				if (terrainGenState->gpuMemoryOriginZ >= (S32)TERRAIN_CHUNK_PER_EDGE) terrainGenState->gpuMemoryOriginZ = 0;
				if (terrainGenState->gpuMemoryOriginZ < 0) terrainGenState->gpuMemoryOriginZ = TERRAIN_CHUNK_PER_EDGE - 1;
				terrainGenState->lastGenerationTriggerZ = view_position.z;

				U32 generationChunkIndexZ = offsetFromCurrentChunkZ > 0 ? (TERRAIN_CHUNK_PER_EDGE - 1) : 0;
				terrainGenState->currentOriginZ += offsetFromCurrentChunkZ > 0 ? 1 : -1;
				for (U32 i = 0; i < TERRAIN_CHUNK_PER_EDGE; i++) {
					GenerateTerrainChunk(terrainGenState, &memory->assets, i, generationChunkIndexZ);
				};
			}
		}
		

		ImGui::Begin("TerrainGenState");
		ImGui::Text("CurrentTerrainOriginX: %d", terrainGenState->currentOriginX);
		ImGui::Text("CurrentTerrainOriginZ: %d", terrainGenState->currentOriginZ);
		ImGui::Text("CurrentOffsetFromChunkX: %f", offsetFromCurrentChunkX);
		ImGui::Text("CurrentOffsetFromChunkZ: %f", offsetFromCurrentChunkZ);
		ImGui::Text("GPUMemoryOriginX: %d", terrainGenState->gpuMemoryOriginX);
		ImGui::Text("GPUMemoryOriginZ: %d", terrainGenState->gpuMemoryOriginZ);
		if (ImGui::Button("Regenerate Terrain"))
		{
			terrainGenState->gpuMemoryOriginX = 0;
			terrainGenState->gpuMemoryOriginZ = 0;
			for (U32 z = 0; z < TERRAIN_CHUNK_PER_EDGE; z++) {
				for (U32 x = 0; x < TERRAIN_CHUNK_PER_EDGE; x++) {
					GenerateTerrainChunk(terrainGenState, &memory->assets, x, z);
				}
			}
		}

		ImGui::End();
	}//TerrainGeneration
#endif

	rs->solidDebugGroup.vertexBlock = rs->vertexBlock;
	rs->solidDebugGroup.indexBlock = rs->indexBlock;

#if 1
	MemoryBlock *block = &memory->mainBlock;
	ProfileData *profileData = &memory->debug_memory.profileData;
	ImGui::Begin("Profiler");
	ImGui::BeginGroup();
	ShowMemoryBlockTree(block);
	ImGui::EndGroup();
	ImGui::BeginGroup();
	for (U32 i = 0; i < profileData->persistantEntryCount; i++)
	{
		PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if (ImGui::CollapsingHeader(entry->name))
		{
			ImGui::Text("Elapsed Time: %f ms", entry->elapsedTimeHistory[profileData->persistantWriteIndex]);
			//ImGui::Text("Elapsed Cycles: %llu", entry->elapsedCycles);
			ImGui::PushID(i);
			ImGui::PlotLines("", entry->elapsedTimeHistory, PROFILER_ELAPSED_TIME_HISTORY_COUNT, profileData->persistantWriteIndex);
			ImGui::PopID();
		}
	}
	

	ImGui::EndGroup();
	ImGui::End();
#endif
	
	GameAssets *assets = &memory->assets;
	ImGui::Begin("Assets");

	ImGui::Columns(2);
	ImGui::Separator();
	ImGui::Text("Materials");
	ImGui::NextColumn();
	ImGui::Text("Info");
	ImGui::NextColumn();
	ImGui::Separator();
	static int selected_index = -1;
	for (U32 i = 0; i < DEBUGMaterialID_COUNT; i++)
	{
		DEBUGMaterialAsset *material = &assets->materials[i];
		if (ImGui::Selectable(material->name, selected_index == (int)i))
			selected_index = i;
	}
	ImGui::NextColumn();

	if (selected_index >= 0)
	{
		DEBUGMaterialAsset *material = &assets->materials[selected_index];
		ImGui::Image((ImTextureID)material->drawable.diffuse_texture_id, ImVec2(256, 256));
		if (material->data.flags & MaterialFlag_NORMAL) {
			ImGui::SameLine();
			ImGui::Image((ImTextureID)material->drawable.normal_texture_id, ImVec2(256, 256));
		}
		if (material->data.flags & MaterialFlag_SPECULAR) {
			ImGui::SameLine();
			ImGui::Image((ImTextureID)material->drawable.specular_texture_id, ImVec2(256, 256));
		}
	}
	ImGui::NextColumn();


	ImGui::End();



	//TODO(Torin) Move simulatePhysics to the top of the
	//game update function
	
	if(!gameplay_settings->use_debug_camera)
	{
		SimulatePhysics(memory);
	}

	//auto pos = V3(25.0f, 4.0f, 25.0f);

	DebugLog *log = &memory->debug_memory.debugLog;	
	static U32 last_entry_count = log->current_entry_count;
	if (last_entry_count != log->current_entry_count) {
		ImGui::SetNextWindowCollapsed(false);
		last_entry_count = log->current_entry_count;
	}

	ImGui::Begin("Console");
	ImGui::BeginGroup();

		for (size_t i = 0; i < log->current_entry_count; i++)
	{
		LogEntry& entry = log->entries[i];
		const V4& color = LOGLEVEL_COLOR[entry.level];
		ImGui::TextColored(ImVec4(color.x, color.y, color.z, color.w), "%s %s", LOGLEVEL_TAG[entry.level], log->entries[i].text);
	}
	ImGui::EndGroup();
	ImGui::End();

	ImGui::Begin("Entities");
	for (size_t i = 0; i < EntityType_COUNT; i++)
	{
		if (ImGui::CollapsingHeader(ENTITY_TYPE_STRING[i]))
		{
			EntityArray *ea = &entities->entityArrays[i];

#define U32_INVALID_INDEX ((U32)(((U64)1 << 32) - 1))

			static U32 selected_index = U32_INVALID_INDEX;
			ImGui::BeginGroup();
			ImGui::BeginChild("0", ImVec2(100.0f, 0.0f), true);
			for (U32 n = 0; n < ea->count; n++)
			{
				ImGui::PushID(n);
				if (ImGui::Selectable("Entity", selected_index == n))
				{
					selected_index = n;
				}
				ImGui::PopID();
			}
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("1");
			if (selected_index != U32_INVALID_INDEX)
			{
				
			}
			ImGui::EndChild();
			ImGui::EndGroup();
		}
	}
	ImGui::End();
}

static inline void DebugRenderPass(GameMemory *memory, const Camera& camera)
{
	RenderState *rs = &memory->renderState;
	TerrainGenerationState *terrainGenState = &memory->terrainGenState;

	M4 model = M4Identity();
	const M4& view = camera.view;
	const M4& proj = camera.projection;
	
	static float elapsedTime = 0.0f;
	elapsedTime += memory->gameState.deltaTime;

	if (rs->debugSettings.render_debug_normals)
	{
		glUseProgram(rs->debugNormalsShader);
		glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);
		glUniform1f(88, elapsedTime);
		//DrawTerrainGeometry(terrainGenState);
		glUseProgram(0);
	}
}

inline bool EpsilonEquals(float a, float b, float e = 0.0001f)
{
	bool result = std::abs(b - a) < e;
	return result;
}

inline bool operator==(const V3& a, const V3& b)
{
	if (std::abs(b.x - a.x) > 0.001f) return false;
	if (std::abs(b.y - a.y) > 0.001f) return false;
	if (std::abs(b.z - a.z) > 0.001f) return false;
	return true;
}

inline bool operator==(const V4& a, const V4& b)
{
	if (std::abs(b.x - a.x) > 0.001f) return false;
	if (std::abs(b.y - a.y) > 0.001f) return false;
	if (std::abs(b.z - a.z) > 0.001f) return false;
	if (std::abs(b.w - a.w) > 0.001f) return false;
	return true;
}


inline float Min(float a, float b)
{
	float result = a < b ? a : b;
	return result;
}

inline float Max(float a, float b)
{
	float result = a > b ? a : b;
	return result;
}



//void UpdateFrustumPoints(Frustum *f, const V3& center, const V3& view_direction)
//{
//	//TODO(Torin) Just pass these in as parameters
//	const V3 up(0.0f, 1.0f, 0.0f);
//	V3 right = Cross(view_direction, up);
//	right = Normalize(right);
//
//	V3 far_plane_center  = center + (view_direction * f->far_plane_distance);
//	V3 near_plane_center = center + (view_direction * f->near_plane_distance);
//
//	float tan_half_fov = tanf(f->field_of_view / 2.0f);
//	float near_half_height = tan_half_fov * f->near_plane_distance;
//	float near_half_width = near_half_height * f->aspect_ratio;
//	float far_half_height = tan_half_fov * f->far_plane_distance;
//	float far_half_width = far_half_height * f->aspect_ratio;
//
//	f->points[0] = near_plane_center - (up * near_half_height) - (right * near_half_width);
//	f->points[1] = near_plane_center + (up * near_half_height) - (right * near_half_width); 
//	f->points[2] = near_plane_center + (up * near_half_height) + (right * near_half_width); 
//	f->points[3] = near_plane_center - (up * near_half_height) + (right * near_half_width); 
//
//	f->points[4] = far_plane_center - (up * far_half_height) - (right * far_half_width); 
//	f->points[5] = far_plane_center + (up * far_half_height) - (right * far_half_width); 
//	f->points[6] = far_plane_center + (up * far_half_height) + (right * far_half_width); 
//	f->points[7] = far_plane_center - (up * far_half_height) + (right * far_half_width); 	
//}

static inline void DrawModelGeometry(const ModelDrawable& drawable)
{
	U64 current_index_offset = 0;
	glBindVertexArray(drawable.vertexArrayID);
	for (U32 i = 0; i < drawable.meshCount; i++)
	{
		glDrawElements(GL_TRIANGLES, drawable.indexCountPerMesh[i], GL_UNSIGNED_INT,
			(GLvoid*)(sizeof(U32) * current_index_offset));
		current_index_offset += drawable.indexCountPerMesh[i];
	}
}

static inline void DrawModelGeometry(const ModelDrawable& drawable, V3 position)
{
	M4 transform_matrix = Translate(position);
	glUniformMatrix4fv(UniformLocation::model, 1, GL_FALSE, &transform_matrix[0][0]);
	DrawModelGeometry(drawable);
}

static inline void DrawModelGeometry(const ModelDrawable& drawable, V3 position, V3 rotation)
{
	M4 transform_matrix = Translate(position) * Rotate(rotation);
	glUniformMatrix4fv(UniformLocation::model, 1, GL_FALSE, &transform_matrix[0][0]);
	DrawModelGeometry(drawable);
}


extern "C" void GameRender(GameMemory *memory)
{
	GameState *gs = &memory->gameState;
	RenderState *rs = &memory->renderState;
	InputState *input = &memory->inputState;
	SystemInfo *system = &memory->systemInfo;

	DebugGameplaySettings *gameplay_settings = &gs->debug_settings;
	Camera *camera = gameplay_settings->use_debug_camera ? &rs->debugCamera : &rs->camera;
	
	//TODO(Torin) Move one of these guys into the render sate
	static float currentTime = 0.0f;
	currentTime += gs->deltaTime;

	glViewport(0, 0, (GLsizei)system->screen_width, (GLsizei)system->screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glStencilMask(0x00);


	auto DrawQuad = [rs]()
	{
		if (rs->quad_vao == 0)
		{
			GLfloat quadVertices[] = {
				// Positions        // Texture Coords
				-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
				1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
				1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
			};
			// Setup plane VAO
			glGenVertexArrays(1, &rs->quad_vao);
			glBindVertexArray(rs->quad_vao);


			glGenBuffers(1, &rs->quad_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, rs->quad_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		}
		glBindVertexArray(rs->quad_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	};


	auto UploadRenderGroup = [](RenderGroup *group)
	{
		glBindVertexArray(group->vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, group->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(U32) * group->current_index_count, 
				group->indexBlock.base, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, group->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DEBUGVertex3D) * group->current_vertex_count, 
				group->vertexBlock.base, GL_DYNAMIC_DRAW);
		glBindVertexArray(0);
	};

	auto SetLightingUniforms = [](const Lighting& lighting, const Camera& camera)
	{
		glUniform3f(UniformLocation::cameraViewPosition, camera.position.x, camera.position.y, camera.position.z);
		glUniform1i(UniformLocation::directionalLightCount, lighting.directionalLightCount);
		glUniform1i(UniformLocation::pointLightCount, lighting.pointLightCount);
		for (int i = 0; i < lighting.directionalLightCount; i++) {
			static const int uniformCountPerDirectionalLight = 2;
			glUniform3f(UniformLocation::directionaLights + 0 + (i*uniformCountPerDirectionalLight), lighting.directionalLights[i].direction.x,
				lighting.directionalLights[i].direction.y, lighting.directionalLights[i].direction.z);
			glUniform3f(UniformLocation::directionaLights + 1 + (i*uniformCountPerDirectionalLight), lighting.directionalLights[i].color.x,
				lighting.directionalLights[i].color.y, lighting.directionalLights[i].color.z);		
		}
	};

	auto DrawRenderGroup = [](RenderGroup *group) 
	{
		glBindVertexArray(group->vao);
		glDrawElements(GL_TRIANGLES, group->current_index_count, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	};
	
	if (rs->debugSettings.is_wireframe_enabled)
	{
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}


	M4 model = M4Identity();
	M4 view = camera->view;
	M4 proj = camera->projection;
	UploadRenderGroup(&rs->solidDebugGroup);
	UploadRenderGroup(&rs->lineDebugGroup);

	if (!rs->debugSettings.is_wireframe_enabled)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glUseProgram(rs->skydome_shader);
		glUniformMatrix4fv(UniformLocation::model, 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(UniformLocation::view, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(UniformLocation::projection, 1, GL_FALSE, &proj[0][0]);
		glBindVertexArray(memory->skydomeVertexArray.vertexArrayID);
		glDrawElements(GL_TRIANGLES, memory->skydomeIndexCount, GL_UNSIGNED_INT, 0);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	GameEntities *entities = &memory->entities;

	glUseProgram(rs->debugShader);
	//model = Rotate(0.0f, entities->player.angle, 0.0f);
	glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);
	DrawRenderGroup(&rs->solidDebugGroup);


	TerrainGenerationState *terrainGenState = &memory->terrainGenState;


	auto BindMaterial = [](const MaterialDrawable& material)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, material.diffuse_texture_id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, material.normal_texture_id);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, material.specular_texture_id);
	};


	auto DrawTerrainGeometry = [memory](TerrainGenerationState *terrainGenState)
	{
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->heightmap_texture_array);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->normals_texture_array);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->detailmap_texture_array);

		glBindVertexArray(terrainGenState->base_mesh.vertexArrayID);
		glDrawElementsInstanced(GL_TRIANGLES, TERRAIN_INDEX_COUNT_PER_CHUNK, 
				GL_UNSIGNED_INT, 0, TERRAIN_TOTAL_CHUNK_COUNT);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	};

	auto DrawTerrainEntitiesWithShading = [&](V3 scale = V3(1.0))
	{
		for (U32 i = 0; i < TERRAIN_TOTAL_CHUNK_COUNT; i++)
		{
			TerrainEntity *chunk_entities = terrainGenState->entityBase + (TERRAIN_ENTITIES_PER_CHUNK * i);
			for (U32 n = 0; n < TERRAIN_ENTITIES_PER_CHUNK; n++)
			{
				TerrainEntity *entity = &chunk_entities[n];
				model = Translate(entity->position);
				model = model * Rotate(entity->rotation);

				const ModelDrawable &drawable = *entity->drawable;
				glBindVertexArray(drawable.vertexArrayID);
				U64 currentIndexOffset = 0;
				for (U32 j = 0; j < drawable.meshCount; j++)
				{
					const MaterialDrawable &material = drawable.materials[j];
					if (material.flags & MaterialFlag_TRANSPARENT)
					{
						glUseProgram(rs->material_transparent_shader);
						glUniformMatrix4fv(UniformLocation::model, 1, GL_FALSE, &model[0][0]);
					} else
					{
						glUseProgram(rs->material_opaque_shader);
						glUniformMatrix4fv(UniformLocation::model, 1, GL_FALSE, &model[0][0]);
					}

					BindMaterial(material);
					glDrawElements(GL_TRIANGLES, drawable.indexCountPerMesh[j], GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
					currentIndexOffset += drawable.indexCountPerMesh[j];
				}
			}
		}
	};






	auto DrawTerrainEntitiesGeometry = [&]()
	{
		for (U32 i = 0; i < TERRAIN_TOTAL_CHUNK_COUNT; i++)
		{
			TerrainEntity *entity = &terrainGenState->entityBase[i];
			model = Translate(entity->position);
			model = model * Rotate(entity->rotation);
			glUniformMatrix4fv(UniformLocation::model, 1, 0, &model[0][0]);

			const ModelDrawable *drawable = entity->drawable;
			DrawModelGeometry(*drawable);
		}
	};

	GameAssets *assets = &memory->assets;
	auto DrawSceneGeometry = [&]()
	{
		DrawTerrainEntitiesGeometry();
		const ModelDrawable& player = GetModelDrawable(assets, DEBUGModelID_player); 
		DrawModelGeometry(player, entities->player.pos, V3(0.0f, entities->player.angle, 0.0f));
	};

#if 0
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
#endif


	{ //Update CSM Cascade Distances
		//TODO(Torin) this could probably be acomplished much simpler with a Lerp
		Frustum *f = rs->csm_cascade_frustums;
		float clip_plane_distance_ratio = camera->far_clip / camera->near_clip;
		f[0].near_plane_distance = camera->near_clip;
		for (U32 i = 1; i < SHADOW_MAP_CASCADE_COUNT; i++) 
		{
			float scalar = (float)i / (float)SHADOW_MAP_CASCADE_COUNT;
			f[i].near_plane_distance = SHADOW_MAP_CASCADE_WEIGHT *
				(camera->near_clip * powf(clip_plane_distance_ratio, scalar)) +
				(1 - SHADOW_MAP_CASCADE_WEIGHT) * (camera->near_clip + (camera->far_clip - camera->near_clip) * scalar);
			f[i - 1].far_plane_distance = f[i].near_plane_distance * SHADOW_MAP_CASCADE_TOLERANCE;
		}
		f[SHADOW_MAP_CASCADE_COUNT - 1].far_plane_distance = camera->far_clip;
	}

	V3 light_direction = rs->lightingState.directionalLights[0].direction;
	M4 light_view = LookAt(V3(0.0f), -light_direction, V3(-1.0f, 0.0f, 0.0f));

	glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
	glBindFramebuffer(GL_FRAMEBUFFER, rs->depth_map_framebuffer);
	glUseProgram(rs->depth_map_shader);

	M4 light_projection;
	M4 light_space_transforms[SHADOW_MAP_CASCADE_COUNT];
	float shadow_cascade_distances[SHADOW_MAP_CASCADE_COUNT];

	for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
	{
		//Compute cascade frustum points	
		Frustum *f = rs->csm_cascade_frustums + i;
		shadow_cascade_distances[i] = 0.5f * (-f->far_plane_distance * proj[2][2] + proj[3][2]) / f->far_plane_distance + 0.5f;

		V3 up = V3(0.0f, 1.0f, 0.0f);
		V3 view_direction = camera->front;
		V3 center = camera->position;
		V3 right = Cross(view_direction, up);

		right = Normalize(right);
		up = Normalize(Cross(right, view_direction));

		V3 far_plane_center = center + (view_direction * f->far_plane_distance);
		V3 near_plane_center = center + (view_direction * f->near_plane_distance);

		float tan_half_fov = tanf(f->field_of_view / 2.0f);
		float near_half_height = tan_half_fov * f->near_plane_distance;
		float near_half_width = near_half_height * f->aspect_ratio;
		float far_half_height = tan_half_fov * f->far_plane_distance;
		float far_half_width = far_half_height * f->aspect_ratio;

		f->points[0] = near_plane_center - (up * near_half_height) - (right * near_half_width);
		f->points[1] = near_plane_center + (up * near_half_height) - (right * near_half_width);
		f->points[2] = near_plane_center + (up * near_half_height) + (right * near_half_width);
		f->points[3] = near_plane_center - (up * near_half_height) + (right * near_half_width);

		f->points[4] = far_plane_center - (up * far_half_height) - (right * far_half_width);
		f->points[5] = far_plane_center + (up * far_half_height) - (right * far_half_width);
		f->points[6] = far_plane_center + (up * far_half_height) + (right * far_half_width);
		f->points[7] = far_plane_center - (up * far_half_height) + (right * far_half_width);

		float minZ, maxZ;
		{
			V4 view_space = light_view * V4(f->points[i], 1.0f);
			minZ = view_space.z;
			maxZ = view_space.z;
		}

		for (int i = 1; i < 8; i++)
		{
			V4 view_space = light_view * V4(f->points[i], 1.0f);
			maxZ = view_space.z > maxZ ? view_space.z : maxZ;
			minZ = view_space.z < minZ ? view_space.z : minZ;
		}

		//TODO(Torin) Insure all loaded objects that cast shadows fall inside the frustum 

		light_projection = Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -maxZ, -minZ, 1.0);
//		light_space_transforms[i] = light_projection * light_view;
		M4 light_transform = light_projection * light_view;

		float maxX = -1000000, minX = 1000000;
		float maxY = -1000000, minY = 1000000;
		for (U32 i = 0; i < 8; i++)
		{
			V4 light_space = light_transform * V4(f->points[i], 1.0f);
			light_space.x /= light_space.w;
			light_space.y /= light_space.w;

			maxX = Max(maxX, light_space.x);
			minX = Min(minX, light_space.x);
			maxY = Max(maxY, light_space.y);
			minY = Min(minY, light_space.y);
		}

		float scaleX = 2.0f / (maxX - minX);
		float scaleY = 2.0f / (maxY - minY);
		float offsetX = -0.5f * (maxX + minX) * scaleX;
		float offsetY = -0.5f * (maxY + minY) * scaleY;

		M4 crop_matrix = M4Identity();
		crop_matrix[0][0] = scaleX;
		crop_matrix[1][1] = scaleY;
		crop_matrix[3][0] = offsetX;
		crop_matrix[3][1] = offsetY;

		light_projection = light_projection * crop_matrix;
		light_transform = light_projection * light_view;
		light_space_transforms[i] = light_transform;

		glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rs->depth_map_texture, 0, i);
		glClear(GL_DEPTH_BUFFER_BIT);
		glUniformMatrix4fv(UniformLocation::light_space, 1, 0, &light_transform[0][0]);

#if 0
		M4 model = M4Identity();
		glUniformMatrix4fv(UniformLocation::model, 1, 0, &model[0][0]);
		glBindVertexArray(terrainGenState->vertexArray.vertexArrayID);
		glDrawElements(GL_TRIANGLES, TERRAIN_TOTAL_CHUNK_COUNT * TERRAIN_INDEX_COUNT_PER_CHUNK * 2, GL_UNSIGNED_INT, 0);
#endif

		DrawSceneGeometry();

	}

	auto SetLightSpaceTransformUniforms = [&light_space_transforms, &shadow_cascade_distances](GLuint programHandle)
	{
		GLint matrix_location = glGetUniformLocation(programHandle, "u_light_space_matrix");
		GLint distance_location = glGetUniformLocation(programHandle, "u_shadow_cascade_distance");
		glUniformMatrix4fv(matrix_location, SHADOW_MAP_CASCADE_COUNT, GL_FALSE, &light_space_transforms[0][0][0]);
		glUniform1fv(distance_location, SHADOW_MAP_CASCADE_COUNT, &shadow_cascade_distances[0]);
	};


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)system->screen_width, (GLsizei)system->screen_height);

	if (rs->debugSettings.render_from_directional_light)
	{
		view = light_view;
		proj = light_projection;
		//glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
	}



	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, rs->depth_map_texture);
	glClear(GL_DEPTH_BUFFER_BIT);

	{ //@CSM
		M4 inverseCameraView = Inverse(camera->view);
		//light_space_transforms[1] = inverseCameraView * light_space_transforms[1];

	}
	
	//light_space_transforms[1] = light_space_transforms[0];

	//Draw terrain
	glUseProgram(rs->terrain_shader);
	SetLightingUniforms(rs->lightingState, *camera);
	SetLightSpaceTransformUniforms(rs->terrain_shader);
	model = M4Identity();
	model = Translate(entities->player.pos);
	glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);

	auto& material0 = GetMaterial(&memory->assets, DEBUGMaterialID_dirt);
	auto& material1 = GetMaterial(&memory->assets, DEBUGMaterialID_grass01);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, material0.diffuse_texture_id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, material1.diffuse_texture_id);
	DrawTerrainGeometry(terrainGenState);

	//Set Material Shaders LightingState Uniforms
	glUseProgram(rs->material_opaque_shader);
	glUniformMatrix4fv(UniformLocation::view, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(UniformLocation::projection, 1, GL_FALSE, &proj[0][0]);
	//glUniformMatrix4fv(UniformLocation::light_space, 1, GL_FALSE, &light_space_transforms[0][0][0]);
	SetLightingUniforms(rs->lightingState, *camera);
	glUseProgram(rs->material_transparent_shader);
	glUniformMatrix4fv(UniformLocation::view, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(UniformLocation::projection, 1, GL_FALSE, &proj[0][0]);
	//glUniformMatrix4fv(UniformLocation::light_space, 1, GL_FALSE, &light_space_transforms[0][0][0]);
	SetLightingUniforms(rs->lightingState, *camera);

	//Draw the static terrain entities
	glEnable(GL_BLEND);
	DrawTerrainEntitiesWithShading();
	glUseProgram(rs->material_opaque_shader);
	DrawModelGeometry(GetModelDrawable(assets, DEBUGModelID_player), entities->player.pos, V3(0.0f, entities->player.angle, 0.0f));


	if (rs->debugSettings.draw_shadowmap_depth_texture)
	{
		glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		glUseProgram(rs->debug_depth_map_shader);
		glUniform1i(0, rs->debugSettings.draw_shadow_map_index);
		DrawQuad();
	}

	glViewport(0, 0, system->screen_width, system->screen_height);

#if 0
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);
	glStencilMask(0xFF);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
#endif

	DebugRenderPass(memory, *camera);

	rs->solidDebugGroup.current_index_count = 0;
	rs->solidDebugGroup.current_vertex_count = 0;

	glUseProgram(rs->debugShader);
	glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);
	glBindVertexArray(rs->lineDebugGroup.vao);
	glDrawElements(GL_TRIANGLES, rs->lineDebugGroup.current_index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	rs->lineDebugGroup.current_index_count = 0;
	rs->lineDebugGroup.current_vertex_count = 0;

	if (rs->debugSettings.is_wireframe_enabled)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	ImGui::Render();
}

