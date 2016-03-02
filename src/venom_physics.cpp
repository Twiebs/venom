struct AABB
{
  V3 min;
  V3 max;
};

struct Sphere
{
  V3 center;
  float radius;
};

struct Capsule
{
  V3 line_segment_a;
  V3 line_segment_b;
  float radius;
};

struct ConvexHull
{
};

struct Plane
{
  V3 normal;
  float distance;
};

Plane ComputePlane(V3 a, V3 b, V3 c)
{
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

V3 ClosestPointOnPlane(V3 a, Plane p)
{
    float t = Dot(p.normal, a) - p.distance;
    V3 result = a - (t * p.normal);
    return result;
};

//==========  Intersection Tests  ================
//=================================================

B32 Intersects(AABB a, AABB b)
{
  if (a.max.x < b.min.x || a.min.x > b.max.x) return 0;
  if (a.max.y < b.min.y || a.min.y > b.max.y) return 0;
  if (a.max.z < b.min.z || a.min.z > b.max.z) return 0;
  return 1;
}

B32 Intersects(Sphere a, Sphere b)
{
  V3 displacement = a.center - b.center;
  float distance_squared = Dot(displacement, displacement);

  float radius_sum = a.radius + b.radius;
  float radius_sum_squared = radius_sum * radius_sum;
  
  int result = distance_squared < radius_sum_squared;
  return result; 
}

B32 Intersects(Capsule a, Sphere b)
{
  return 1;
}

//================================================

Sphere ComputeBoundingSphere(MeshData data)
{
  Sphere result = {};

  float max_distance_squared = 0;
  fori(data.vertexCount)
  {
	auto vertex = data.vertices[i];
	max_distance_squared = std::abs(MagnitudeSquared(vertex.position));	
  }

  result.center = V3(0.0f);
  result.radius = sqrt(max_distance_squared);
  return result;
}


AABB ComputeAABB(const MeshData& data)
{
  AABB result = {};
  return result;
}
  
ConvexHull ComputeConvexHull(const MeshData& data)
{
  ConvexHull result = {};
  return result;
} 
