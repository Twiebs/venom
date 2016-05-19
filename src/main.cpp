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

#include "game_assets.h"
#include "venom_platform.h"
#include "venom_physics.cpp"
#include "venom_render.cpp"
#include "venom_module.cpp"

#include "game.h"

#include "debug_imgui.cpp"

#ifdef VENOM_HOTLOAD
#ifndef VENOM_RELEASE
#include "venom_debug.cpp"
#endif//VENOM_RELEASE
#endif//VENOM_HOTLOAD

#include "venom_noise.cpp"
#include "math_procedural.cpp"

#include "terrain.cpp"

#include "opengl_render.cpp"

static void GenerateQuadByExtrudingLineSegment(V3 start, V3 end, F32 lengthY, 
    Vertex3D* vertices, U32* indices, U32* currentVertexCount, 
    U32* currentIndexCount, U32 maxVertexCount, U32 maxIndexCount) 
{
  assert(*currentVertexCount + 4 < maxVertexCount);
  assert(*currentIndexCount  + 6 < maxIndexCount);

  U8* rawVertexData = (U8*)vertices;

  F32 lengthX = std::abs(end.x - start.x);
  F32 lengthZ = std::abs(end.z - start.z);

  vertices[0].position = start;
  vertices[1].position = end;
  vertices[2].position = end + V3 { 0, lengthY, 0 };
  vertices[3].position = start + V3 { 0, lengthY, 0 }; 

  vertices[0].texcoord = V2 { 0.0f, 0.0f };
  vertices[1].texcoord = V2 { 4.0f, 0.0f };
  vertices[2].texcoord = V2 { 4.0f, 2.0f};
  vertices[3].texcoord = V2 { 0.0f, 2.0f};

  indices[0] = *currentVertexCount + 0;
  indices[1] = *currentVertexCount + 1;
  indices[2] = *currentVertexCount + 2;
  indices[3] = *currentVertexCount + 0;
  indices[4] = *currentVertexCount + 2;
  indices[5] = *currentVertexCount + 3;

  *currentVertexCount += 4;
  *currentIndexCount += 6;
}

struct Rectangle {
  F32 minX, minY;
  F32 maxX, maxY; 
};

static void GenerateStructure(V3 origin, RNGSeed& seed, IndexedVertexArray* vertexArray) {
  Vertex3D vertices[512];
  U32 indices[(512 / 4) * 6];
  U32 vertexCount = 0;
  U32 indexCount = 0;

  F32 structureHeightOffset = origin.y;
  F32 wallHeight = 20.0f;

  F32 baseHalfWidth = RandomInRange(6.2f, 23.0f, seed);
  F32 baseHalfLength = RandomInRange(6.2f, 23.0f, seed);
  Rectangle bounds = { origin.x - baseHalfWidth, origin.z - baseHalfLength,
    origin.x + baseHalfWidth, origin.z + baseHalfLength};

  V3 start = { bounds.minX, structureHeightOffset, bounds.minY };
  V3 end = { bounds.minX, structureHeightOffset, bounds.maxY };
  GenerateQuadByExtrudingLineSegment(start, end, wallHeight,
    &vertices[vertexCount], &indices[indexCount], 
    &vertexCount, &indexCount,
    ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
  start = { bounds.minX, structureHeightOffset, bounds.maxY};
  end = { bounds.maxX, structureHeightOffset, bounds.maxY };
  GenerateQuadByExtrudingLineSegment(start, end, wallHeight,
    &vertices[vertexCount], &indices[indexCount], 
    &vertexCount, &indexCount,
    ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
  start = { bounds.maxX, structureHeightOffset, bounds.maxY};
  end = { bounds.maxX, structureHeightOffset, bounds.minY};
  GenerateQuadByExtrudingLineSegment(start, end, wallHeight,
    &vertices[vertexCount], &indices[indexCount], 
    &vertexCount, &indexCount,
    ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
  start = { bounds.maxX, structureHeightOffset, bounds.minY};
  end = { bounds.minX, structureHeightOffset, bounds.minY };
  GenerateQuadByExtrudingLineSegment(start, end, wallHeight,
    &vertices[vertexCount], &indices[indexCount], 
    &vertexCount, &indexCount,
    ARRAY_COUNT(vertices), ARRAY_COUNT(indices));

  CalculateSurfaceNormals(vertices, vertexCount, indices, indexCount);
  CreateIndexedVertexArray3D(vertexArray, vertexCount, indexCount,
    vertices, indices, GL_STATIC_DRAW);
}

void VenomModuleStart(GameMemory *memory) {
  RenderState *rs = &memory->renderState;
  GameAssets *assets = &memory->assets;
  SystemInfo *sys = &memory->systemInfo;

  float far_plane_distance = (TERRAIN_CHUNK_PER_EDGE*0.5)* TERRAIN_CHUNK_SIZE;
  InitializeCamera(&rs->camera, 45.0f * DEG2RAD, 0.1f, far_plane_distance);
  rs->camera.position = V3(0.0f, 1.0f, 0.0f);
  
  GameEntities *entities = &memory->entities;
  entities->player.pos = V3(TERRAIN_ORIGIN_TO_CENTER_CHUNK_OFFSET *TERRAIN_CHUNK_SIZE,
    0.001, TERRAIN_ORIGIN_TO_CENTER_CHUNK_OFFSET*TERRAIN_CHUNK_SIZE);
  InitalizeTerrainGenerator(&memory->terrainGenState, 
    &memory->mainBlock, entities->player.pos);

  { //@Skydome Init
    static const int skydomeResolution = 8;
    MeshData skydomeMeshData;
    GetSubdiviedCubeVertexAndIndexCount(skydomeResolution, 
      &skydomeMeshData.vertexCount, &skydomeMeshData.indexCount);
    skydomeMeshData.vertices = ReserveArray(Vertex3D, 
        skydomeMeshData.vertexCount, &memory->mainBlock);
    skydomeMeshData.indices = ReserveArray(U32, 
      skydomeMeshData.indexCount, &memory->mainBlock);
    GenerateSubdiviedCubeMeshData(&skydomeMeshData,skydomeResolution);
    for (U32 i = 0; i < skydomeMeshData.vertexCount; i++) {
      skydomeMeshData.vertices[i].position = 
        Normalize(skydomeMeshData.vertices[i].position);
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

    for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
      rs->csm_cascade_frustums[i].aspect_ratio = 
        sys->screen_width / sys->screen_height;
      rs->csm_cascade_frustums[i].field_of_view = rs->camera.fov + 0.2f;
    }
  }

  { //@Graphics 
    //TODO(Torin) Move out the imgui and debug render groups
    //To the venom module so they are reusable
  
    rs->terrain_shader = GetShaderProgram(assets, ShaderID_terrain);
    rs->skydome_shader = GetShaderProgram(assets, ShaderID_skydome);
    rs->depth_map_shader = GetShaderProgram(assets, ShaderID_depth_map);
    rs->material_opaque_shader = GetShaderProgram(assets, ShaderID_material_opaque);
    rs->material_transparent_shader = 
      GetShaderProgram(assets, ShaderID_material_transparent);

    rs->lightingState.directionalLights[0].direction = V3(0.2f, 0.9f, 0.0f);
    rs->lightingState.directionalLights[0].color = V3(0.9f, 0.9f, 0.9f);
    rs->lightingState.directionalLightCount = 1;
  }
}

static B32  
RandomBSPTree(const Rectangle& node, const U32 targetSplitCount, 
  const U32 currentTreeDepth, RNGSeed& seed, 
  Rectangle* results, U32* currentResultCount)
{
  F32 nodeWidth  = node.maxX - node.minX;
  F32 nodeLength = node.maxY - node.minY;
  Rectangle childA;
  Rectangle childB;

  U64 splitDirection = RandomInt01(seed);
  if (splitDirection == 0) {
    F32 minSplitSize = nodeWidth * 0.33F;
    F32 splitSize = minSplitSize + RandomInRange(0.0F, 0.33F, seed);
    childA = { node.minX, node.minY, node.minX + splitSize, node.maxY };
    childB = { childA.maxX, node.minY, node.maxX, node.maxY };
  } else if (splitDirection == 1) {
    F32 minSplitSize = nodeLength * 0.33F;
    F32 splitSize = minSplitSize + RandomInRange(0.0F, 0.33F, seed);
    childA = { node.minX, node.minY, node.maxX, node.minY + splitSize };
    childB = { node.minX, childA.maxY, node.maxX, node.maxY };
  }

  if (currentTreeDepth == targetSplitCount) {
    U64 childSelection = RandomInt01(seed);
    results[*currentResultCount] = childSelection ? childA : childB;
    *currentResultCount += 1;
    return 1;
  } else {
    if (RandomBSPTree(childA, targetSplitCount, currentTreeDepth + 1, 
      seed, results, currentResultCount)) return 0;
    if (RandomBSPTree(childB, targetSplitCount, currentTreeDepth + 1, 
      seed, results, currentResultCount)) return 0;
    return 0;
  }
}

V2 Center(const Rectangle& rectangle) {
  F32 width = rectangle.maxX - rectangle.minX;
  F32 length = rectangle.maxY - rectangle.minY;
  V2 result = { rectangle.minX + (width * 0.5F),
    rectangle.minY + (length * 0.5F) };
   return result; 
}

void VenomModuleLoad(GameMemory *memory)
{	
#if 1
	TerrainGenerationState *terrainGenState = &memory->terrainGenState;
	for (U32 z = 0; z < TERRAIN_CHUNK_PER_EDGE; z++) 
		for (U32 x = 0; x < TERRAIN_CHUNK_PER_EDGE; x++)
			GenerateTerrainChunk(terrainGenState, &memory->assets, x, z);
#endif

  RNGSeed seed(2);

  U32 targetSplitCount = 3;
  U32 finalResultCount = pow(2, targetSplitCount) / 2;
  Rectangle rootBSPNode = { 512, 512, 600, 600 };
  Rectangle results[finalResultCount];
  U32 currentResultCount = 0;
  RandomBSPTree(rootBSPNode, targetSplitCount, 0, seed, results, &currentResultCount);
  assert(currentResultCount == finalResultCount);

  memory->entities.structureCount = finalResultCount;
  for (size_t i = 0; i < finalResultCount; i++) {
    V2 center = Center(results[i]);
    GenerateStructure( V3 { center.x, 20, center.y }, 
      seed, &memory->entities.structures[i]);
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

void VenomModuleUpdate(GameMemory *memory)
{
	GameState *gameState = &memory->gameState;
	DebugGameplaySettings *gameplay_settings = &gameState->debug_settings;
	InputState *input = &memory->inputState;
	RenderState *rs = &memory->renderState;
	GameEntities *entities = &memory->entities;

	//imgui_update_state(memory);
	//ImGui::NewFrame();

	if (input->isKeyDown[KEYCODE_CTRL] && input->isKeyDown[KEYCODE_Q])
	   gameState->isRunning = false;
	
	if (input->toggleDebugModePressed) {
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


	if (!gameplay_settings->use_debug_camera) { 
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

	else if (input->isButtonDown[MOUSE_RIGHT]) {
		MoveCameraWithFPSControls(memory, camera);
	}

  ShowCameraInfo(camera);
  ImGui::Begin("DebugGameplaySettings");
	ImGuiBoolEdit(gameplay_settings->disable_terrain_generation);
	ImGuiBoolEdit(gameplay_settings->use_debug_camera);
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


#if 0
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
				for (U64 i = 0; i < TERRAIN_CHUNK_PER_EDGE; i++) {
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
				for (U64 i = 0; i < TERRAIN_CHUNK_PER_EDGE; i++) {
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
		if (ImGui::Button("Regenerate Terrain")) {
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

  ShowProfiler(&memory->debug_memory.profileData,&memory->mainBlock); 
  ShowAssets(&memory->assets);
  ShowConsole(&memory->debug_memory.debugLog);

	//TODO(Torin) Move simulatePhysics to the top of the
	//game update function

#if 0  
	if(!gameplay_settings->use_debug_camera) {
		SimulatePhysics(memory);
	}


	//auto pos = V3(25.0f, 4.0f, 25.0f);

	DebugLog *log = &memory->debug_memory.debugLog;	
	static U32 last_entry_count = log->current_entry_count;
	if (last_entry_count != log->current_entry_count) {
		ImGui::SetNextWindowCollapsed(false);
		last_entry_count = log->current_entry_count;
	}
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



static inline void DebugRenderPass(GameMemory *memory, const Camera& camera) {
	RenderState *rs = &memory->renderState;
	TerrainGenerationState *terrainGenState = &memory->terrainGenState;

	M4 model = M4Identity();
	const M4& view = camera.view;
	const M4& proj = camera.projection;
	
	static float elapsedTime = 0.0f;
	elapsedTime += memory->gameState.deltaTime;

	if (rs->debugSettings.render_debug_normals) {
		glUseProgram(rs->debugNormalsShader);
		glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);
		glUniform1f(88, elapsedTime);
		DrawTerrainGeometry(terrainGenState);
		glUseProgram(0);
	}
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


void VenomModuleRender(GameMemory *memory)
{
	GameState *gs = &memory->gameState;
	RenderState *rs = &memory->renderState;
	InputState *input = &memory->inputState;
	SystemInfo *system = &memory->systemInfo;
  GameAssets* assets = &memory->assets;

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

	auto SetLightingUniforms = [](const Lighting& lighting, const Camera& camera) {
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

	auto DrawTerrainEntitiesWithShading = [&](V3 scale = V3(1.0)) {
		for (U64 i = 0; i < TERRAIN_TOTAL_CHUNK_COUNT; i++) {
			TerrainEntity *chunk_entities = terrainGenState->entityBase + 
        (TERRAIN_ENTITIES_PER_CHUNK * i);
			for (U64 n = 0; n < TERRAIN_ENTITIES_PER_CHUNK; n++) {
				TerrainEntity *entity = &chunk_entities[n];
				model = Translate(entity->position);
				model = model * Rotate(entity->rotation);
        auto drawable = GetModelDrawable(assets, (DEBUGModelID)entity->modelID);
        DrawModelWithShading(drawable, model, rs);
      }
    }
	};

	auto DrawTerrainEntitiesGeometry = [&]() {
		for (U64 i = 0; i < TERRAIN_TOTAL_CHUNK_COUNT; i++) {
			TerrainEntity *entity = &terrainGenState->entityBase[i];
			model = Translate(entity->position);
			model = model * Rotate(entity->rotation);
			glUniformMatrix4fv(UniformLocation::model, 1, 0, &model[0][0]);
			const ModelDrawable &drawable = GetModelDrawable(assets, 
        (DEBUGModelID)entity->modelID);
			DrawModelGeometry(drawable);
		}
	};

	auto DrawSceneGeometry = [&]() {
		DrawTerrainEntitiesGeometry();
		const ModelDrawable& player = GetModelDrawable(assets, DEBUGModelID_player); 
		DrawModelGeometry(player, entities->player.pos, 
      V3(0.0f, -entities->player.angle, 0.0f));
    for (U32 i = 0; i < entities->structureCount; i++) {
      model = M4Identity();
      glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
      glBindVertexArray(entities->structures[i].vertexArrayID);
      glDrawElements(GL_TRIANGLES, entities->structures[i].indexCount, 
        GL_UNSIGNED_INT, 0); 
      glBindVertexArray(0);
    }
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
		for (U32 i = 1; i < SHADOW_MAP_CASCADE_COUNT; i++) {
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

	if (rs->debugSettings.render_from_directional_light) {
		view = light_view;
		proj = light_projection;
		//glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
	}



	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, rs->depth_map_texture);
	glClear(GL_DEPTH_BUFFER_BIT);

  { //@Draw @Terrain 
    glUseProgram(rs->terrain_shader);
    SetLightingUniforms(rs->lightingState, *camera);
    SetLightSpaceTransformUniforms(rs->terrain_shader);
    model = M4Identity();
    model = Translate(entities->player.pos);
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);

    //TODO(Torin) Give the terrain a better material system
    auto& material0 = GetMaterial(&memory->assets, MaterialID_dirt00);
    auto& material1 = GetMaterial(&memory->assets, MaterialID_grass01);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material0.diffuse_texture_id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material1.diffuse_texture_id);
    DrawTerrainGeometry(terrainGenState);
  }

  //NOTE(Torin) For now set the shader uniforms in advance
  //because there is no async gpu submision pipeline and draw calls
  //are executed imediatly
	glUseProgram(rs->material_opaque_shader);
	glUniformMatrix4fv(UniformLocation::view, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(UniformLocation::projection, 1, GL_FALSE, &proj[0][0]);
	SetLightingUniforms(rs->lightingState, *camera);
	glUseProgram(rs->material_transparent_shader);
	glUniformMatrix4fv(UniformLocation::view, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(UniformLocation::projection, 1, GL_FALSE, &proj[0][0]);
	SetLightingUniforms(rs->lightingState, *camera);
  glUseProgram(0);

  { //Render the scene with shading
    glEnable(GL_BLEND);
    DrawTerrainEntitiesWithShading();
    glUseProgram(rs->material_opaque_shader);
    DrawModelGeometry(GetModelDrawable(assets, DEBUGModelID_player), 
      entities->player.pos, V3(0.0f, -entities->player.angle * 1.0, 0.0f)); 
    glUseProgram(rs->material_opaque_shader);
    for (U32 i = 0; i < entities->structureCount; i++) {
      BindMaterial(GetMaterial(assets, MaterialID_floor_wood01));
      model = M4Identity();
      glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
      glBindVertexArray(entities->structures[i].vertexArrayID);
      glDrawElements(GL_TRIANGLES, entities->structures[i].indexCount, 
        GL_UNSIGNED_INT, 0); 
      glBindVertexArray(0);
    }
  }
 
	if (rs->debugSettings.draw_shadowmap_depth_texture) {
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

	rs->solidDebugGroup.current_index_count = 0;
	rs->solidDebugGroup.current_vertex_count = 0;

	glUseProgram(rs->debugShader);
  model = M4Identity(); 
	glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &proj[0][0]);
	glBindVertexArray(rs->lineDebugGroup.vao);
	glDrawElements(GL_TRIANGLES, rs->lineDebugGroup.current_index_count, GL_UNSIGNED_INT, 0);

	DebugRenderPass(memory, *camera);

  rs->lineDebugGroup.current_index_count = 0;
	rs->lineDebugGroup.current_vertex_count = 0;

	if (rs->debugSettings.is_wireframe_enabled) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//ImGui::Render();
}

