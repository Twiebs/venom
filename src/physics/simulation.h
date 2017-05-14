
#if 0
struct PhysicsObjectState {
  V3 position;
  V3 momentum;

  V3 angularVelocity;
  V3 linearVelocity;

  F32 mass;
  F32 inverseMass;
};
#endif

struct BroadphaseVolume {
  AABB aabb;
  EntityIndex entityID;
  B8 isIntersected;
};

struct TerrainGenerationState;

struct PhysicsSimulation {
  DynamicArray<BroadphaseVolume> dynamicBodies;
  DynamicArray<BroadphaseVolume> staticBodies;

  V3 gravityAcceleration;

  //DynamicArray<CollisionVisualization> collisionVisualizations;
  TerrainGenerationState *terrain;
  CollisionVisualization collisionVisualization;
};





static inline void SimulatePhysics(PhysicsSimulation *sim, EntityContainer *entities);