
struct CollisionInfo {
  V3 penetrationVector;
};

struct GJKStep {
  V3 simplex[4];
  size_t simplexCount;
};

struct EPATriangle {
  V3 a, b, c;
  V3 normal;
};

struct EPAStep {
  EPATriangle closestTriangle;
  EPATriangle triangles[128];
  size_t triangleCount;
};

struct CollisionVisualization {
  AABB a, b;
  DynamicArray<GJKStep> gjkSteps;
  DynamicArray<EPAStep> epaSteps;
  V3 penetrationVector;

  //TODO(Torin 2017-01-31) Should this be here?
  B8 showPenetrationVector; 
  B8 showClosestTriangle;
  B8 hideMinkowskiRegion;
  B8 renderSolidTriangles;

  void AddGJKStep(V3 *simplex, size_t simplexCount) {
    auto gjkStep = gjkSteps.AddElement();
    memcpy(gjkStep->simplex, simplex, simplexCount * sizeof(V3));
    gjkStep->simplexCount = simplexCount;
  }

  void AddEPAStep(EPATriangle closestTriangle, EPATriangle *triangles, size_t triangleCount) {
    auto epaStep = epaSteps.AddElement();
    epaStep->closestTriangle = closestTriangle;
    assert(triangleCount < ARRAY_COUNT(EPAStep::triangles));
    for (size_t i = 0; i < triangleCount; i++) {
      EPATriangle& t = triangles[i];
      if (Equals(t.a, closestTriangle.a) &&
        Equals(t.b, closestTriangle.b) &&
        Equals(t.c, closestTriangle.c)) continue;
      epaStep->triangles[i] = triangles[i];
      epaStep->triangleCount++;
    }
  }
};


inline B32 TestAABBAABBIntersection(const AABB& a, const AABB& b);
inline B32 TestSphereSphereIntersection(const Sphere& a, const Sphere& b);
inline B32 TestLineTriangleIntersection(V3 lineStart, V3 lineEnd, V3 a, V3 b, V3 c, V3 *result);



inline B32 TestSphereTriangle(const Sphere& s, const V3& a, const V3& b, const V3& c);
int IntersectRayAABB(V3 p, V3 d, AABB a, float &tmin, V3 &q);