
struct Quaternion {
  F32 x, y, z, w;
};

inline Quaternion operator+(Quaternion& a, Quaternion& b) {
  Quaternion result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  result.w = a.w + b.w;
  return result;
}

inline Quaternion operator*(Quaternion& a, Quaternion& b) {
  Quaternion result;
  result.w = (a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);
  result.x = (a.w*b.x + a.x*b.w - a.y*b.z + a.z*b.y);
  result.y = (a.w*b.y + a.x*b.z + a.y*b.w - a.z*b.x);
  result.z = (a.w*b.z - a.x*b.y + a.y*b.x + a.z*b.w);
  return result;
}

inline Quaternion operator*(Quaternion a, float b) {
  Quaternion result;
  result.x = a.x * b;
  result.y = a.y * b;
  result.z = a.z * b;
  result.w = a.w * b;
  return result;
}

inline float Dot(Quaternion a, Quaternion b) {
  return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

inline float Magnitude(Quaternion q) {
  return sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
}

inline Quaternion Normalize(Quaternion q) {
  float magnitude = Magnitude(q);
  Quaternion result;
  result.x = q.x / magnitude;
  result.y = q.y / magnitude;
  result.z = q.z / magnitude;
  result.w = q.w / magnitude;
  return result;
}

inline Quaternion Lerp(Quaternion& a, Quaternion& b, float t) {
  float dot = Dot(a, b);
  float bias = t > 0.0f ? 1.0f : -1.0f;
  Quaternion result = (b * t) + (a * (bias * (1.0f - t)));
  return result;
}

inline Quaternion QuaternionFromEulerAngles(F32 x, F32 y, F32 z) {
  F32 cx = cos(x*0.5f);
  F32 cy = cos(y*0.5f);
  F32 cz = cos(z*0.5f);
  F32 sx = sin(x*0.5f);
  F32 sy = sin(y*0.5f);
  F32 sz = sin(z*0.5f);

  Quaternion q;
  q.w = cx * cy * cz + sx * sy * sz;
  q.x = sx * cy * cz - cx * sy * sz;
  q.y = cx * sy * cz + sx * cy * sz;
  q.z = cx * cy * sz - sx * sy * cz;
  return q;
}

#if 1
inline M4 QuaternionToMatrix(Quaternion& quaternion) {
  Quaternion q = Normalize(quaternion);

  F32 x2 = q.x*q.x;
  F32 y2 = q.y*q.y;
  F32 z2 = q.z*q.z;
  F32 w2 = q.w*q.w;

  M4 r;


  r[0][0] = w2 + x2 - y2 - z2;
  r[0][1] = (2*q.x*q.y) + (2*q.w*q.z);
  r[0][2] = (2*q.x*q.z) - (2*q.w*q.y);
  r[0][3] = 0;

  r[1][0] = (2 * q.x*q.y) - (2 * q.w*q.z);
  r[1][1] = (w2 - x2 + y2 - z2);
  r[1][2] = (2 * q.y*q.z) - (2 * q.w*q.x);
  r[1][3] = 0;

  r[2][0] = (2 * q.x*q.z) + (2 * q.w*q.y);
  r[2][1] = (2 * q.y*q.z) + (2 * q.w*q.x);
  r[2][2] = w2 - x2 - y2 + z2;
  r[2][3] = 0;

  r[3][0] = 0;
  r[3][1] = 0;
  r[3][2] = 0;
  r[3][3] = 1;

  return r;
}
#else



#endif

