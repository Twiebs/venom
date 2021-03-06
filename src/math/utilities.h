template<typename T>
inline void Swap(T* a, T* b) {
  T t = *a;
  *a = *b;
  *b = t;
}

template <typename T>
inline T Clamp(T value, T min, T max) {
  if (value > max) return max;
  if (value < min) return min;
  return value;
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

template <typename T>
T min(T a, T b) {
  T r0 = a < b ? a : b;
  return r0;
}

template <typename T>
T max3(T a, T b, T c) {
  T r0 = a > b ? a : b;
  T r1 = c > r0 ? c : r0;
  return r1;
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
