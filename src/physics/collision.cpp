
inline B32 TestAABBAABBIntersection(const AABB& a, const AABB& b) {
  if (a.max.x < b.min.x || a.min.x > b.max.x) return 0;
  if (a.max.y < b.min.y || a.min.y > b.max.y) return 0;
  if (a.max.z < b.min.z || a.min.z > b.max.z) return 0;
  return 1;
}

inline B32 TestSphereSphereIntersection(const Sphere& a, const Sphere& b) {
  V3 displacement = a.center - b.center;
  F32 distance_squared = Dot(displacement, displacement);
  F32 radius_sum = a.radius + b.radius;
  F32 radius_sum_squared = radius_sum * radius_sum;
  B32 result = distance_squared < radius_sum_squared;
  return result;
}

inline B32 TestLinePlaneIntersection(V3 lineStart, V3 lineEnd, V3 planePoint, V3 planeNormal, V3 *result) {
  V3 direction = lineEnd - lineStart;
  F32 planeDistance = Dot(planeNormal, planePoint);
  F32 t = (planeDistance - Dot(planeNormal, lineStart)) / Dot(planeNormal, direction);
  if (t >= 0.0f && t <= 1.0f) {
    *result = lineStart + t * direction;
    return true;
  }

  return false;
}

inline B32 TestLineTriangleIntersection(V3 lineStart, V3 lineEnd, V3 a, V3 b, V3 c, V3 *result) {
  const V3& p = lineStart;
  const V3& q = lineEnd;
  V3 pq = q - p;
  V3 pa = a - p;
  V3 pb = b - p;
  V3 pc = c - p;

  F32 u = TrippleProduct(pq, pc, pb);
  if (u < 0.0f) return 0;
  F32 v = TrippleProduct(pq, pa, pc);
  if (v < 0.0f) return 0;
  F32 w = TrippleProduct(pq, pb, pa);
  if (w < 0.0f) return 0;

  F32 scale = 1.0f / (u + v + w);
  u *= scale;
  v *= scale;
  w *= scale;

  *result = u*a + v*b + w*c;
  return true;
}

inline B32 TestSphereTriangleIntersection(V3 center, F32 radius, const V3& a, const V3& b, const V3& c, V3 *peneterationVector) {
  V3 p = ClosestPointOnTriangle(center, a, b, c);
  V3 displacement = p - center;
  F32 distance = Magnitude(displacement);
  if (distance <= radius) {
    V3 direction = Normalize(displacement);
    F32 diff = radius - distance;
    *peneterationVector = direction * diff;
    return true;
  }

  return false;
}

inline B32 TestIntersection(V3 line_begin, V3 line_end, Plane plane, V3 *intersection_point) {
  V3 line_vector = line_end - line_begin;
  F32 t = (plane.distance - Dot(plane.normal, line_begin)) / Dot(plane.normal, line_vector);
  if (t >= 0.0f && t <= 1.0f) {
    *intersection_point = line_begin + (t * line_vector);
    return 1;
  }
  return 0;
}

inline int IntersectRayAABB(V3 p, V3 d, AABB a, float &tmin, V3 &q) {
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

B32 TestTerrainRayIntersection(Terrain *terrain, V3 origin, V3 direction, V3 *result) {
  for (size_t i = 0; i < terrain->chunkCount; i++) {
    TerrainChunk *chunk = &terrain->chunks[i];
    glBindBuffer(GL_ARRAY_BUFFER, chunk->vertexBufferID);
    TerrainVertex *vertices = (TerrainVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    if (vertices == nullptr) return false;

    for (size_t j = 0; j < terrain->vertexCountPerChunk; j+=3) {
      if (TestLineTriangleIntersection(origin, origin + (direction * 10000.0f),
        vertices[j + 0].position, vertices[j + 1].position, vertices[j + 2].position,result)) {
        glUnmapBuffer(GL_ARRAY_BUFFER);
        return true;
      }
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
  }

  return false;
}

B32 TestTerrainSphereIntersection(Terrain *terrain, V3 center, F32 radius, V3 *penetrationVector) {
  S32 chunkXCoord = center.x / terrain->chunkWidthInMeters;
  S32 chunkZCoord = center.z / terrain->chunkWidthInMeters;
  U64 chunkIndex = (chunkZCoord * terrain->terrainWidthInChunks) + chunkXCoord;
  TerrainChunk *chunk = &terrain->chunks[chunkIndex];
  glBindBuffer(GL_ARRAY_BUFFER, chunk->vertexBufferID);
  TerrainVertex *vertices = (TerrainVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  assert(vertices != 0);

  for (size_t i = 0; i < terrain->vertexCountPerChunk; i+=3) {
    if (TestSphereTriangleIntersection(center, radius, vertices[i].position, vertices[i + 1].position, vertices[i + 2].position, penetrationVector)) {
      glUnmapBuffer(GL_ARRAY_BUFFER);
      return true;
    }
  }

  glUnmapBuffer(GL_ARRAY_BUFFER);
  return false;
}

inline V3 GJKArbitraryPolytopeSupport(V3 *a, size_t countA, size_t strideA, V3 *b, size_t countB, size_t strideB, V3 direction) {
  V3 p0 = GetMostDistantPointAlongDirection(a, countA, strideA, direction);
  V3 p1 = GetMostDistantPointAlongDirection(b, countB, strideB, -direction);
  V3 minkowskiDifference = p0 - p1;
  return minkowskiDifference;
}

struct GJKState {
  V3 simplex[4];
  size_t simplexCount;
  V3 searchDirection;

  void SetSimplex(V3 p0) {
    simplex[0] = p0;
    simplexCount = 1;
    auto& vis = GetEngine()->physicsSimulation.collisionVisualization;
    vis.AddGJKStep(simplex, simplexCount);
  }

  void SetSimplex(V3 p0, V3 p1) {
    simplex[0] = p0;
    simplex[1] = p1;
    simplexCount = 2;
    auto& vis = GetEngine()->physicsSimulation.collisionVisualization;
    vis.AddGJKStep(simplex, simplexCount);
  }

  void SetSimplex(V3 p0, V3 p1, V3 p2) {
    simplex[0] = p0;
    simplex[1] = p1;
    simplex[2] = p2;
    simplexCount = 3;
    auto& vis = GetEngine()->physicsSimulation.collisionVisualization;
    vis.AddGJKStep(simplex, simplexCount);
  }

  void SetSearchDirection(V3 direction) {
    searchDirection = direction;
    if (Equals(searchDirection, V3(0.0f))) {
      LogError("GJK Set searchDirection to [0, 0, 0]");
      GetEngine()->isPaused = true;
    }
  }
};

inline B8 GJKIsSameDirection(V3 a, V3 b) {
  F32 dot = Dot(a, b);
  return dot > 0.0f;
}

//UVW is CCW
inline B32 GJKTetrahedronTriangleCase(GJKState *gjkState, V3 u, V3 v, V3 w) {
  const V3 uo = -u;
  const V3 uv = v - u;
  const V3 uw = w - u;
  V3 uvw = Cross(uv, uw);

  if (GJKIsSameDirection(Cross(uv, uvw), uo)) {
    if (GJKIsSameDirection(uv, uo)) {
      gjkState->SetSimplex(v, u);
      gjkState->SetSearchDirection(Cross(Cross(uv, uo), uv));
    } else {
      if (Dot(uw, uo) > 0.0f) {
        gjkState->SetSimplex(w, u);
        gjkState->SetSearchDirection(Cross(Cross(uw, uo), uw));
      } else {
        gjkState->SetSimplex(u);
        gjkState->SetSearchDirection(uo);
      }
    }
  } else {
    if (Dot(Cross(uvw, uw), uo) > 0.0f) {
      if (Dot(uw, uo) > 0.0f) {
        gjkState->SetSimplex(w, u);
        gjkState->SetSearchDirection(Cross(Cross(uw, uo), uw));
      } else {
        gjkState->SetSimplex(u);
        gjkState->SetSearchDirection(uo);
      }
    } else {
      if (Dot(uvw, uo) > 0.0f) {
        return true;
      } else {
        gjkState->SetSimplex(w, v, u);
        gjkState->SetSearchDirection(-uvw);
      }
    }
  }

  return false;
}

inline B32 GJKUpdateSimplex(GJKState *gjkState) {
  switch (gjkState->simplexCount) {

    case 2: /* Line Segment */ {
      const V3& a = gjkState->simplex[1];
      const V3& b = gjkState->simplex[0];
      const V3 ao = -a;
      const V3 ab = b - a;

      if (Dot(ab, ao) > 0.0f) {
        gjkState->SetSearchDirection(Cross(Cross(ab, ao), ab));
      } else {
        gjkState->SetSearchDirection(ao);
        gjkState->SetSimplex(a);
      }
      
      return false;
    } break;

    case 3: /* Triangle */ {
      const V3& a = gjkState->simplex[2];
      const V3& b = gjkState->simplex[1];
      const V3& c = gjkState->simplex[0];

      const V3 ao = -a;
      const V3 ab = b - a;
      const V3 ac = c - a;
      V3 normal = Cross(ac, ab);

      if (Dot(Cross(ac, normal), ao) > 0.0f) {
        if (Dot(ac, ao) > 0.0f) {
          gjkState->SetSimplex(c, a);
          gjkState->SetSearchDirection(Cross(Cross(ac, ao), ac));
        } else {
          if (Dot(ab, ao) > 0.0f) {
            gjkState->SetSimplex(b, a);
            gjkState->SetSearchDirection(Cross(Cross(ab, ao), ab));
          } else {
            gjkState->SetSimplex(a);
            gjkState->SetSearchDirection(ao);
          }
        }
      } else {
        if (Dot(Cross(normal, ab), ao) > 0.0f) {
          if (Dot(ab, ao) > 0.0f) {
            gjkState->SetSimplex(b, a);
            gjkState->SetSearchDirection(Cross(Cross(ab, ao), ab));
          } else {
            gjkState->SetSimplex(a);
            gjkState->SetSearchDirection(ao);
          }
        } else {
          if (Dot(normal, ao) > 0.0f) {
            gjkState->SetSearchDirection(normal);
          } else {
            gjkState->SetSimplex(b, c, a);
            gjkState->SetSearchDirection(-normal);
          }
        }
      }
      return false;
    } break;

    case 4: /* Tetrahedron */ {
      //BCD was our previous ACB so we don't check that one

      const V3& a = gjkState->simplex[3];
      const V3& b = gjkState->simplex[2];
      const V3& c = gjkState->simplex[1];
      const V3& d = gjkState->simplex[0];

      V3 ao = -a;

      //ADB
      if (GJKTetrahedronTriangleCase(gjkState, a, d, b) == false) {
        return false;
      }

      //ACD
      if (GJKTetrahedronTriangleCase(gjkState, a, c, d) == false) {
        return false;
      }

      //ABC
      if (GJKTetrahedronTriangleCase(gjkState, a, b, c) == false) {
        return false;
      }
      
      
      //The origin is contained within the tetrahedron!
      return true;

    } break;

  }

}

static inline V3 EPAClosestPointToOrigin(const EPATriangle& t) {
  F32 distance = Dot(t.normal, -t.a);
  V3 result = -distance * t.normal;
  return result;
}

static inline V3 EPAClosestPointOnTriangle(const V3 p, const EPATriangle& t) {
  F32 distance = Dot(t.normal, p - t.a);
  V3 result = p - distance * t.normal;
  return result;
}


//TODO(Torin) For now we will just return a 
//penetration vector but we need to generate a contact manifold in the future
inline V3 EPAFromGJKSimplex(V3 *initalSimplex, V3 *a, size_t countA, size_t strideA, V3 *b, size_t countB, size_t strideB) {

  struct EPAEdge {
    V3 a, b;
  };

  static const size_t MAX_EDGE_COUNT = 256;
  static const size_t MAX_TRIANGLE_COUNT = 128;

  EPATriangle triangles[MAX_TRIANGLE_COUNT] = {};
  EPAEdge edges[MAX_EDGE_COUNT] = {};
  size_t triangleCount = 0;
  size_t edgeCount = 0;

  auto& vis = GetEngine()->physicsSimulation.collisionVisualization;

  auto AddTriangle = [&](V3 a, V3 b, V3 c) {
    EPATriangle& triangle = triangles[triangleCount++];
    triangle.a = a;
    triangle.b = b;
    triangle.c = c;

    V3 ab = b - a;
    V3 ac = c - a;
    triangle.normal = Cross(ab, ac);
    triangle.normal = Normalize(triangle.normal);
    //assert(Dot(triangle.normal, -a) < 0.0f);
  };

  auto RemoveTriangle = [&](size_t i) {
    assert(i < triangleCount);
    triangles[i] = triangles[triangleCount - 1];
    triangleCount -= 1;
  };

  auto AddOrRemoveEdge = [&](V3 a, V3 b) {
    for (size_t i = 0; i < edgeCount; i++) {
      auto& edge = edges[i];
      if (Equals(edge.a, b) && Equals(edge.b, a)) {
        edges[i] = edges[edgeCount - 1];
        edgeCount--;
        return;
      }
    }

    EPAEdge& edge = edges[edgeCount++];
    edge.a = a;
    edge.b = b;
  };

  { //Add inital simplex to the polytope
    const V3& a = initalSimplex[3];
    const V3& b = initalSimplex[2];
    const V3& c = initalSimplex[1];
    const V3& d = initalSimplex[0];
    AddTriangle(b, c, d);
    AddTriangle(a, b, d);
    AddTriangle(a, d, c);
    AddTriangle(a, c, b);
    EPATriangle dummy = {};
    vis.AddEPAStep(dummy, triangles, triangleCount);
  }
  
  static size_t const MAX_ITERATIONS = 6;
  size_t iterationCount = 0;
  while (iterationCount < MAX_ITERATIONS) {
    //Get closest face of the polytope to the origin
    F32 minMagnitudeSquared = 999999.0f; //@TODO(Torin) FLT_MAX 
    size_t closestFaceIndex = 0;
    for (size_t i = 0; i < triangleCount; i++) {
      EPATriangle& t = triangles[i];
      V3 closestPoint = EPAClosestPointToOrigin(t);
      F32 magnitudeSquared = MagnitudeSquared(closestPoint);
      if (magnitudeSquared < minMagnitudeSquared) {
        minMagnitudeSquared = magnitudeSquared;
        closestFaceIndex = i;
      }
    }

    const EPATriangle closestTriangle = triangles[closestFaceIndex];
    const V3 extendPoint = GJKArbitraryPolytopeSupport(a, countA, strideA, b, countB, strideB, closestTriangle.normal);
    V3 closestPointOnTriangle = EPAClosestPointOnTriangle(extendPoint, closestTriangle);
    if (MagnitudeSquared(Abs(extendPoint - closestPointOnTriangle)) < 0.01f) {
      //This is the closest face to the mankowski difference
      V3 penetrationVector = Project(closestPointOnTriangle, closestTriangle.normal);
      vis.penetrationVector = penetrationVector;
      return penetrationVector;
    } else {

      //Expand the Polytope

      //TODO(Torin) @HACK How should this actualy be done?
      B8 markedForRemoval[128];
      memset(markedForRemoval, 0x00, 128);
      assert(triangleCount < 128);
      for (size_t i = 0; i < triangleCount; i++) {
        EPATriangle triangle = triangles[i];
        F32 dot = Dot(triangle.normal, extendPoint - triangle.a);
        if (dot > 0.0f) {
          markedForRemoval[i] = true;
          AddOrRemoveEdge(triangle.a, triangle.b);
          AddOrRemoveEdge(triangle.b, triangle.c);
          AddOrRemoveEdge(triangle.c, triangle.a);
        }
      }

      //@Hack???  Mabye this isn't such a bad idea...
      size_t newTriangleCount = 0;
      for (size_t i = 0; i < triangleCount; i++) {
        if (markedForRemoval[i] == false) {
          triangles[newTriangleCount++] = triangles[i];
        }
      }

      triangleCount = newTriangleCount;


      for (size_t i = 0; i < edgeCount; i++) {
        auto& edge = edges[i];
        AddTriangle(edge.a, edge.b, extendPoint);
      }
      vis.AddEPAStep(closestTriangle, triangles, triangleCount);
      edgeCount = 0;
    }
    iterationCount++;
  }

  if (iterationCount == MAX_ITERATIONS) {
    LogError("EPA Reached max iterrations");
    GetEngine()->isPaused = true;
    return V3(0.0f);
  }
}

inline B32 GJKIntersection(const AABB& a, const AABB& b, CollisionInfo *collison) {
  V3 aabbA[8], aabbB[8];
  AABBToVertices(a, aabbA, sizeof(V3));
  AABBToVertices(b, aabbB, sizeof(V3));
  
  GJKState gjkState = {};

  auto sim = &GetEngine()->physicsSimulation;
  CollisionVisualization *vis = &sim->collisionVisualization;
  vis->a = a;
  vis->b = b;

  V3 initalPoint = GJKArbitraryPolytopeSupport(aabbA, 8, sizeof(V3), aabbB, 8, sizeof(V3), V3(1.0f, 0.0f, 0.0f));
  gjkState.simplex[gjkState.simplexCount++] = initalPoint;
  vis->AddGJKStep(gjkState.simplex, gjkState.simplexCount);
  gjkState.searchDirection= -initalPoint;

  static const size_t GJK_MAX_ITERATIONS = 32;
  size_t iterationCount = 0;
  while (iterationCount < GJK_MAX_ITERATIONS) {
    V3 p = GJKArbitraryPolytopeSupport(aabbA, 8, sizeof(V3), aabbB, 8, sizeof(V3), gjkState.searchDirection);
    F32 dot = Dot(p, gjkState.searchDirection);
    if (dot < 0.0f) return false;
    gjkState.simplex[gjkState.simplexCount++] = p; 
    vis->AddGJKStep(gjkState.simplex, gjkState.simplexCount);
    if (GJKUpdateSimplex(&gjkState)) {
      assert(gjkState.simplexCount == 4);
      V3 penetration = EPAFromGJKSimplex(gjkState.simplex, aabbA, 8, sizeof(V3), aabbB, 8, sizeof(V3));
      //V3 penetration = V3(0.0f);
      collison->penetrationVector = penetration;
      return true;
    }

    if (MagnitudeSquared(gjkState.searchDirection) < 0.01f) {
      assert(false);
      return true;
    }

    iterationCount++;
  }

  if (iterationCount == GJK_MAX_ITERATIONS) {
    LogError("GJK Reached Max Iterrations");
    GetEngine()->isPaused = true;
  }


  return false;
}



