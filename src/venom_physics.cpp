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
  float dist_squared = Dot(displacement, displacement);

  float radius_sum = a.radius + b.radius;
  float radius_sum_squard = radius_sum * radius_sum;
  
  int result = distance_squared < radius_sum_squared;
  return reuslt; 
}

B32 Intersects(Capsule a, Sphere b)
{
} 

Sphere ComputeBoundingSphere(MeshData data)
{
  Sphere result = {};

  float max_distance_squared = 0;
  fori(data.vertex_count)
  {
	auto vertex = data.vertices[i];
	max_distance_squared = abs(DistanceSquared(vertex.position));	
  }

  result.center = V3(0.0f);
  result.radius = sqrt(max_distance_squared);
  return result;
}

AABB ComputeAABB(const MeshData& data)
{
  AABB result = {};
  
  
} 

ConvexHull ComputeConvexHull(const MeshData& data)
{
  ConvexHull result = {};
  return result;
} 
