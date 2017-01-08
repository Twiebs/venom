
struct AABB {
  V3 min;
  V3 max;
  AABB() {}
  AABB(V3 min, V3 max) : min(min), max(max) {}
};

struct Sphere {
  V3 center;
  float radius;
};

struct Capsule {
  V3 line_segment_a;
  V3 line_segment_b;
  float radius;
};

struct Plane {
  V3 normal;
  float distance;
};



AABB ComputeAABB(const MeshData* data);



inline V3 Center(const AABB aabb) {
  V3 size = Abs(aabb.max - aabb.min);
  V3 result = aabb.min + (size * 0.5f);
  return result;
}
