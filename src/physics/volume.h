
struct AABB {
  V3 min;
  V3 max;
  AABB() {}
  AABB(V3 min, V3 max) : min(min), max(max) {}
};

struct Sphere {
  V3 center;
  F32 radius;
};

struct Triangle {
  V3 a, b, c;
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

struct ConvexHull {
  V3 *points;
};


V3 ClosestPointOnTriangle(V3 p, V3 a, V3 b, V3 c);


inline V3 Center(const AABB& aabb);
inline V3 Size(const AABB& aabb);
inline AABB MinkowskiDifference(const AABB& a, const AABB& b);
AABB ComputeAABB(V3 *positions, size_t count, size_t stride);


Plane ComputePlane(V3 a, V3 b, V3 c);
V3 ClosestPointOnPlane(V3 a, Plane p);


ConvexHull ComputeConvexHull(V3 *positions, size_t count, size_t stride);

Sphere ComputeBoundingSphere(AnimatedVertex *vertices, size_t count);