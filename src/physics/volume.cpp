
inline V3 Size(const AABB& aabb) {
  V3 result = aabb.max - aabb.min;
  return result;
}

AABB ComputeAABB(V3 *positions, size_t count, size_t stride) {
  AABB result = {};

  for (size_t i = 0; i < count; i++) {
    V3 *position = (V3 *)((uintptr_t)positions + (stride*i));
    result.min.x = Min(result.min.x, position->x);
    result.min.y = Min(result.min.y, position->y);
    result.min.z = Min(result.min.z, position->z);
    result.max.x = Max(result.max.x, position->x);
    result.max.y = Max(result.max.y, position->y);
    result.max.z = Max(result.max.z, position->z);
  }
  return result;
}

inline AABB MinkowskiDifference(const AABB& a, const AABB& b) {
  V3 newMin = a.min - b.max;
  V3 fullSize = Size(a) + Size(b);
  AABB result = { newMin, newMin + fullSize };
  return result;
}

//@Optimize: Naive Version!
inline V3 GetMostDistantPointAlongDirection(V3 *a, size_t count, size_t stride, V3 direction) {
  strict_assert(count > 0);
  F32 maxDotProduct = -99999999.0f;
  V3 mostDistantPoint;
  for (size_t i = 0; i < count; i++) {
    V3 p = *(V3 *)((uintptr_t)a + stride*i);
    F32 dot = Dot(p, direction);
    if (dot > maxDotProduct) {
      maxDotProduct = dot;
      mostDistantPoint = p;
    }
  }

  return mostDistantPoint;
}

void AABBToVertices(const AABB& a, V3 *positions, size_t stride) {
  *(V3 *)((uintptr_t)positions + stride * 0) = V3(a.min.x, a.min.y, a.min.z);
  *(V3 *)((uintptr_t)positions + stride * 1) = V3(a.max.x, a.min.y, a.min.z);
  *(V3 *)((uintptr_t)positions + stride * 2) = V3(a.max.x, a.min.y, a.max.z);
  *(V3 *)((uintptr_t)positions + stride * 3) = V3(a.min.x, a.min.y, a.max.z);
  *(V3 *)((uintptr_t)positions + stride * 4) = V3(a.min.x, a.max.y, a.min.z);
  *(V3 *)((uintptr_t)positions + stride * 5) = V3(a.max.x, a.max.y, a.min.z);
  *(V3 *)((uintptr_t)positions + stride * 6) = V3(a.max.x, a.max.y, a.max.z);
  *(V3 *)((uintptr_t)positions + stride * 7) = V3(a.min.x, a.max.y, a.max.z);
}

Sphere ComputeBoundingSphere(AnimatedVertex *vertices, size_t count) {
  Sphere result = {};
  F32 max_distance_squared = 0;
  for (U32 i = 0; i < count; i++) {
    auto vertex = vertices[i];
    max_distance_squared = std::abs(MagnitudeSquared(vertex.position));
  }

  result.center = V3(0.0f);
  result.radius = sqrt(max_distance_squared);
  return result;
}

Plane ComputePlane(V3 a, V3 b, V3 c) {
  Plane result;
  result.normal = Normalize(Cross(b - a, c - a));
  result.distance = Dot(result.normal, a);
  return result;
}

V3 ClosestPointOnPlane(V3 point, V3 a, V3 b, V3 c) {
  V3 normal = Normalize(Cross(b - a, c - a));
  F32 distance = Dot(normal, a);
  F32 t = Dot(normal, point) - distance;
  V3 result = point - (t * normal);
  return result;
}

V3 ClosestPointOnPlane(V3 a, Plane p) {
  float t = Dot(p.normal, a) - p.distance;
  V3 result = a - (t * p.normal);
  return result;
};

V3 ClosestPointOnPlane(V3 p, V3 normal, F32 distance) {
  F32 t = Dot(normal, p) - distance;
  V3 result = p - (t * normal);
  return result;
}

V3 ClosestPointOnTriangle(V3 p, V3 a, V3 b, V3 c) {
  V3 ab = b - a;
  V3 ac = c - a;
  V3 bc = c - b;

  F32 snom = Dot(p - a, ab);
  F32 sdenom = Dot(p - b, a - b);

  F32 tnom = Dot(p - a, ac);
  F32 tdenom = Dot(p - c, a - c);

  if (snom < 0.0f && tnom < 0.0f) return a;

  F32 unom = Dot(p - b, bc);
  F32 udenom = Dot(p - c, b - c);

  if (sdenom < 0.0f && unom <= 0.0f) return b;
  if (tdenom < 0.0f && udenom <= 0.0f) return c;

  V3 n = Cross(ab, ac);

  F32 vc = Dot(n, Cross(a - p, b - p));
  if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f)
    return a + snom / (snom + sdenom) * ab;

  F32 va = Dot(n, Cross(b - p, c - p));
  if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f)
    return b + unom / (unom + udenom) * bc;

  F32 vb = Dot(n, Cross(c - p, a - p));
  if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f)
    return a + tnom / (tnom + tdenom) * ac;

  F32 u = va / (va + vb + vc);
  F32 v = vb / (va + vb + vc);
  F32 w = 1.0f - u - v;
  return u * a + v * b + w * c;
}

inline V3 Center(const AABB& aabb) {
  V3 size = Abs(aabb.max - aabb.min);
  V3 result = aabb.min + (size * 0.5f);
  return result;
}

AABB LocalAABBToWorldAABB(AABB local, V3 worldTranslation, Matrix3x3& worldRotation) {
  AABB result;
  for (size_t i = 0; i < 3; i++) {
    result.min[i] = worldTranslation[i];
    result.max[i] = worldTranslation[i];
    for (size_t j = 0; j < 3; j++) {
      F32 e = worldRotation[i][j] * local.min[j];
      F32 f = worldRotation[i][j] * local.max[j];
      if (e < f) {
        result.min[i] += e;
        result.max[i] += f;
      } else {
        result.min[i] += f;
        result.max[i] += e;
      }
    }
  }
  return result;
}

void QuickHull3D(V3 *positions, size_t count, size_t stride) {



}