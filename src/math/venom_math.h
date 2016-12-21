static const float PI32 = 3.141592653589793238462643383f;
static const float DEG2RAD = (PI32 / 180.0f);
static const float RAD2DEG = (180.0f / PI32);

struct Quaternion {
  F32 w, x, y, z;
};

struct M4 {
  F32 data[4][4];

  inline const float* operator[](int i) const {
    strict_assert(i >= 0);
    strict_assert(i < 4);
    return (const float*)&data[i];
  }

  inline float* operator[](int i) {
    strict_assert(i >= 0);
    strict_assert(i < 4);
    return (float*)&data[i];
  }
};

inline M4 M4Identity();

#include <math.h>
#include "math/utilities.h"
#include "math/vector.h"
#include "math/quaternion.h"
#include "math/matrix.h"
#include "math/geometry.h"
#include "math/random.h"