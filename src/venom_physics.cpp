

static Plane 
ComputePlane(V3 a, V3 b, V3 c) {
  Plane result;
  result.normal = Normalize(Cross(b - a, c - a));
  result.distance = Dot(result.normal, a);
  return result;
}

static inline
int IntersectsPlane(V3 line_begin, V3 line_end, Plane plane, V3* intersection_point)
{
    V3 line_vector = line_end - line_begin;
    float t = (plane.distance - Dot(plane.normal, line_begin)) / Dot(plane.normal, line_vector);

    if (t >= 0.0f && t <= 1.0f)
    {
        *intersection_point = line_begin + (t * line_vector);
        return 1;
    }
    return 0;
}

inline V3 ClosestPointOnPlane(V3 a, Plane p) {
    float t = Dot(p.normal, a) - p.distance;
    V3 result = a - (t * p.normal);
    return result;
};

inline B32 Intersects(AABB a, AABB b) {
  if (a.max.x < b.min.x || a.min.x > b.max.x) return 0;
  if (a.max.y < b.min.y || a.min.y > b.max.y) return 0;
  if (a.max.z < b.min.z || a.min.z > b.max.z) return 0;
  return 1;
}



#if 0
static int
IntersectRayAABB(V3 origin, V3 direction, AABB aabb) {
  float tmin = (aabb.min.x - origin.x) / direction.x;
  float tmax = (aabb.max.x - origin.x) / direction.x;
  if (tmin > tmax) Swap(tmin, tmax);

  float tymin = (aabb.min.y - origin.y) / direction.y;
  float tymax = (aabb.max.y - origin.y) / direction.y;
  if (tymin > tymax) Swap(tymin, tymax);

  if ((tmin > tymax) || (tymin > tmax)) return 0;
  
  if (tymin > tmin) tmin = tymin;
  if (tymax < tmax) tmax = tymax;

  float tzmin = (aabb.min.z - origin.z) / direction.z;
  float tzmax = (aabb.max.z - origin.z) / direction.z;
  if (tzmin > tzmax) Swap(tzmin, tzmax);
  
  if ((tmin > tzmax) || (tzmin > tmax)) return 0;
  if (tzmin > tmin) tmin = tzmin;
  if (tzmax < tmax) tmax = tzmax;

  return 1; 
}
#endif

#if 0
static int
IntersectRayAABB(V3 origin, V3 direction, AABB aabb) {
  float tx0 = (aabb.min.x - origin.x) / direction.x;
  float tx1 = (aabb.max.x - origin.x) / direction.x;
  if(tx0 > tx1) Swap(tx0, tx1);

  float ty0 = (aabb.min.y - origin.y) / direction.y;
  float ty1 = (aabb.max.y - origin.y) / direction.y;
  if(ty0 > ty1) Swap(ty0, ty1);

  float tmin = tx0, tmax = tx1;
  if((tmin > ty1) || (ty0 > tmax)) return 0;

  if(ty0 > tmin) tmin = ty0;
  if(ty1 > tmax) tmax = ty1;

  float tz0 = (aabb.min.z - origin.z) / direction.z;
  float tz1 = (aabb.max.z - origin.z) / direction.z;
  if(tz0 > tz1) Swap(tz0, tz1);

  if((tmin > tz1) || (tz0 > tmax)) return 0;
  return 1;  
}
#endif

int IntersectRayAABB(V3 p, V3 d, AABB a, float &tmin, V3 &q)
{
  tmin = 0.0f;
  // set to -FLT_MAX to get first hit on line
  float tmax = FLT_MAX;
  // set to max distance ray can travel (for segment)  
  // For all three slabs
  for (int i = 0; i < 3; i++) {
    if (abs(d[i]) < FLT_EPSILON) {
    // Ray is parallel to slab. No hit if origin not within slab
    if (p[i] < a.min[i] || p[i] > a.max[i]) return 0;
    } else {
    // Compute intersection t value of ray with near and far plane of slab
    float ood = 1.0f / d[i];
    float t1 = (a.min[i] - p[i]) * ood;
    float t2 = (a.max[i] - p[i]) * ood;
    // Make t1 be intersection with near plane, t2 with far plane
    if (t1 > t2) Swap(&t1, &t2);
    // Compute the intersection of slab intersection intervals
    if (t1 > tmin) tmin = t1;
    if (t2 < tmax) tmax = t2;
    // Exit with no collision as soon as slab intersection becomes empty
    if (tmin > tmax) return 0;
    }
  }
  // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
  q = p + d * tmin;
  return 1;
}

#if 0
static B32
IntersectRayAABB(V3 rayOrigin, V3 rayDirection, AABB aabb)
{
  float tmin = 0.0f;
  float tmax = FLT_MAX;
  for (U64 i = 0; i < 3; i++) {
    if (abs(rayDirection[i]) < FLT_EPSILON) {
      if (rayOrigin[i] < aabb.min[i] || rayOrigin[i] > aabb.max[i]) return 0;
    } else {
      F32 ood = 1.0f / rayDirection[i];
      F32 t1 = (aabb.min[i] - rayOrigin[i]) * ood;
      F32 t2 = (aabb.max[i] - rayOrigin[i]) * ood;
      if(t1 > t2) Swap(t1, t2);
      if(t1 > tmin) tmin = t1;
      if(t2 > tmax) tmax = t2;
      if(tmin > tmax) return 0;
    }
  }
  return 1;
}
#endif

static B32 Intersects(Sphere a, Sphere b) {
  V3 displacement = a.center - b.center;
  float distance_squared = Dot(displacement, displacement);

  float radius_sum = a.radius + b.radius;
  float radius_sum_squared = radius_sum * radius_sum;
  
  int result = distance_squared < radius_sum_squared;
  return result; 
}
//================================================

static Sphere 
ComputeBoundingSphere(MeshData data) 
{
  Sphere result = {};
  F32 max_distance_squared = 0;
  for (U32 i = 0; i < data.vertexCount; i++) {
    auto vertex = data.vertices[i];
    max_distance_squared = std::abs(MagnitudeSquared(vertex.position));	
  }

  result.center = V3(0.0f);
  result.radius = sqrt(max_distance_squared);
  return result;
}


AABB ComputeAABB(const MeshData* data) {
  AABB result = {};
  for (size_t i = 0; i < data->vertexCount; i++) {
    AnimatedVertex *vertex = (AnimatedVertex *)&data->vertices[i];
    result.min.x = Min(result.min.x, vertex->position.x);
    result.min.y = Min(result.min.y, vertex->position.y);
    result.min.z = Min(result.min.z, vertex->position.z);
    result.max.x = Max(result.max.x, vertex->position.x);
    result.max.y = Max(result.max.y, vertex->position.y);
    result.max.z = Max(result.max.z, vertex->position.z);
  }
  return result;
}

void UpdateEntityPhysics(Entity *entity, F32 deltaTime) {
  entity->position += entity->velocity * deltaTime;
  //entity->velocity *= 0.01f; //TODO(Torin) Drag for material
}