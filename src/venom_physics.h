struct AABB {
  V3 min;
  V3 max;
};

AABB ComputeAABB(const MeshData* data);
