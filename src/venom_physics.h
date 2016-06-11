struct AABB {
  V3 min;
  V3 max;
};

inline V3 
Center(const AABB aabb) {
  V3 size = Abs(aabb.max - aabb.min);
  V3 result = aabb.min + (size * 0.5f);
  return result;
}

AABB ComputeAABB(const MeshData* data);
