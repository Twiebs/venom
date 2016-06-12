static const float PI32 = 3.141592653589793238462643383f;
static const float DEG2RAD = (PI32 / 180.0f);
static const float RAD2DEG = (180.0f / PI32);

#include <math.h>
#include "math_vector.h"
#include "math_matrix.h"
#include "math_geometry.h"
#include "math_procedural.h"

template<typename T>
inline void Swap(T* a, T* b){
  T t = *a;
  *a = *b;
  *b = t;
}

inline F32 
Clamp(F32 value, F32 min, F32 max) {
	if (value > max) return max;
	if (value < min) return min;
	return value;
}

inline F32 
Clamp01(F32 value) {
	if (value > 1.0f) return 1.0f;
	if (value < 0.0f) return 0.0f;
	return value;
}

inline S64 
ClampInt(S64 value, S64 min, S64 max) {
	if (value > max) return max;
	if (value < min) return min;
	return value;
}

inline U32 
ClampU32(U32 value, U32 min, U32 max) {
	if (value > max) return max;
	if (value < min) return min;
	return value;
}

inline F32 
Min(F32 a, F32 b) {
	F32 result = a < b ? a : b;
	return result;
}

inline F32 
Max(F32 a, F32 b) {
	F32 result = a > b ? a : b;
	return result;
}

inline B8 
Equals(F32 a, F32 b, F32 e = 0.0001f) {
	bool result = std::abs(b - a) < e;
	return result;
}

inline float 
Lerp(float a, float b, float t) {
	float result = a + t * (b - a);
	return result;
}
