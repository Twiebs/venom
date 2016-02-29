#pragma once 

static const float PI32 = 3.141592653589793238462643383f;
static const float DEG2RAD = (PI32 / 180.0f);
static const float RAD2DEG = (180.0f / PI32);

#include <math.h>

#include "math_vector.h"
#include "math_matrix.h"
#include "math_geometry.h"
#include "math_procedural.h"

inline float Clamp(float value, float min, float max)
{
	if (value > max) return max;
	if (value < min) return min;
	return value;
}

inline float Clamp01(float value)
{
	if (value > 1.0f) return 1.0f;
	if (value < 0.0f) return 0.0f;
	return value;
}

inline int ClampInt(int value, int min, int max)
{
	if (value > max) return max;
	if (value < min) return min;
	return value;
}

inline U32 ClampU32(U32 value, U32 min, U32 max)
{
	if (value > max) return max;
	if (value < min) return min;
	return value;
}

inline float Lerp(float a, float b, float t)
{
	float result = a + t * (b - a);
	return result;
}

struct Quaternion
{
	float x, y, z, w;
};