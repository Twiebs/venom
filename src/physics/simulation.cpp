
static inline void SimulatePhysics(PhysicsSimulation *sim, EntityContainer *entities) {
  static const F32 timeStep = 1.0f / 60.0f;
  assert(sim->dynamicBodies.count == 0);
  assert(sim->staticBodies.count == 0);
  sim->collisionVisualization.gjkSteps.count = 0;
  sim->collisionVisualization.epaSteps.count = 0;
  Engine *engine = GetEngine();

  BeginTimedBlock("BroadPhaseVolumes");
  ItterateEntities(entities, [sim, engine](Entity *entity, EntityIndex entityIndex) {
    ModelAsset *model = GetModelAsset(entity->modelID);
    if (model == nullptr) return;
    BroadphaseVolume *volume = nullptr;
    if (entity->type == EntityType_StaticObject) {
      volume = sim->staticBodies.AddElement();
    } else {
      volume = sim->dynamicBodies.AddElement();

#if 1
      F32 terrainHeight = GetTerrainHeightAtPosition(&engine->terrain, entity->position);
      if (entity->position.y > terrainHeight) {
        entity->velocity += sim->gravityAcceleration * timeStep;
      }

      entity->position += entity->velocity * timeStep;
      if (entity->position.y < terrainHeight) {
        entity->position.y = terrainHeight;
        entity->velocity.y = 0.0f;
      } 

#endif
      
    }

    Matrix3x3 rotation = QuaterionToMatrix3x3(entity->rotation);
    volume->aabb = LocalAABBToWorldAABB(model->aabb, entity->position, rotation);
    volume->aabb.min.y += model->size.y * 0.5f;
    volume->aabb.max.y += model->size.y * 0.5f;
    volume->entityID = entityIndex;

  });
  EndTimedBlock("BroadPhaseVolumes");

  BeginTimedBlock("BroadPhaseIntersection");
  for (size_t i = 0; i < sim->dynamicBodies.count; i++) {
    BroadphaseVolume &a = sim->dynamicBodies[i];
    if (a.isIntersected == true) continue;

    for (size_t j = 0; j < sim->dynamicBodies.count; j++) {
      if (i == j) continue;
      BroadphaseVolume &b = sim->dynamicBodies[j];
      if (b.isIntersected) continue;
      if (TestAABBAABBIntersection(a.aabb, b.aabb)) {
        assert(false);
        Entity *e = GetEntity(a.entityID, entities);
        e->velocity = V3(0.0f);
        a.isIntersected = true;
        b.isIntersected = true;
      }
    }

    for (size_t j = 0; j < sim->staticBodies.count; j++) {
      BroadphaseVolume &b = sim->staticBodies[j];
      if (b.isIntersected) continue;
      if (TestAABBAABBIntersection(a.aabb, b.aabb)) {

        CollisionInfo collision = {};
        if (GJKIntersection(a.aabb, b.aabb, &collision)) {
          Entity *e = GetEntity(a.entityID, entities);
          e->position -= collision.penetrationVector;
          a.isIntersected = true;
          b.isIntersected = true;
        }
      }
    }
  }

  EndTimedBlock("BroadPhaseIntersection");


#ifndef VENOM_RELEASE
  VenomDebugRenderSettings *settings = GetDebugRenderSettings();
  if (settings->drawBroadphaseVolumes) {
    for (size_t i = 0; i < sim->dynamicBodies.count; i++) {
      BroadphaseVolume &a = sim->dynamicBodies[i];
      V4 color = a.isIntersected ? COLOR_RED : COLOR_GREEN;
      draw_debug_box(a.aabb.min, a.aabb.max, color);
    }

    for (size_t i = 0; i < sim->staticBodies.count; i++) {
      BroadphaseVolume &a = sim->staticBodies[i];
      V4 color = a.isIntersected ? COLOR_RED : COLOR_GREEN;
      draw_debug_box(a.aabb.min, a.aabb.max, color);
    }
  }
#endif//VENOM_RELEASE

  sim->dynamicBodies.count = 0;
  sim->staticBodies.count = 0;
}