
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


inline M4 QuaternionToMatrix(Quaternion& q) {
#if 0
  M4 r;

  F32 x2 = q.x*q.x;
  F32 y2 = q.y*q.y;
  F32 z2 = q.z*q.z;
  F32 w2 = q.w*q.w;

  r[0][0] = 1 - (2 * y2) - (2 * z2);
  r[0][1] = (2 * q.x * q.y) - (2 * q.w * q.z);
  r[0][2] = (2 * q.x * q.z) + (2 * q.w * q.y);
  r[0][3] = 0.0f;

  r[1][0] = (2 * q.x * q.y) + (2 * q.w * q.z);
  r[1][1] = 1.0f - (2.0f * x2) - (2.0f * z2);
  r[1][2] = (2 * q.y * q.z) - (2 * q.w * q.x);
  r[1][3] = 0.0f;

  r[2][0] = (2 * q.x * q.z) - (2 * q.w * q.y);
  r[2][1] = (2 * q.y * q.z) + (2 * q.w * q.x);
  r[2][2] = 1.0f - (2 * x2) - (2 * y2);
  r[2][3] = 0.0f;

  r[3][0] = 0.0f;
  r[3][1] = 0.0f;
  r[3][2] = 0.0f;
  r[3][3] = 1.0f;

  return r;

#else //https://github.com/g-truc/glm/blob/master/glm/gtc/quaternion.inl
  M4 Result = M4Identity();
  F32 qxx(q.x * q.x);
  F32 qyy(q.y * q.y);
  F32 qzz(q.z * q.z);
  F32 qxz(q.x * q.z);
  F32 qxy(q.x * q.y);
  F32 qyz(q.y * q.z);
  F32 qwx(q.w * q.x);
  F32 qwy(q.w * q.y);
  F32 qwz(q.w * q.z);

  Result[0][0] = F32(1) - F32(2) * (qyy + qzz);
  Result[0][1] = F32(2) * (qxy + qwz);
  Result[0][2] = F32(2) * (qxz - qwy);

  Result[1][0] = F32(2) * (qxy - qwz);
  Result[1][1] = F32(1) - F32(2) * (qxx + qzz);
  Result[1][2] = F32(2) * (qyz + qwx);

  Result[2][0] = F32(2) * (qxz + qwy);
  Result[2][1] = F32(2) * (qyz - qwx);
  Result[2][2] = F32(1) - F32(2) * (qxx + qyy);
  return Result;
#endif
}




inline V3 QuaternionToEuler(const Quaternion& q) {
//https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  V3 result;
  F32 ysqr = q.y * q.y;
  // roll (x-axis rotation)
  F32 t0 = +2.0f * (q.w * q.x + q.y * q.z);
  F32 t1 = +1.0f - 2.0f * (q.x * q.x + ysqr);
  result.x = std::atan2(t0, t1);
  // pitch (y-axis rotation)
  F32 t2 = +2.0f * (q.w * q.y - q.z * q.x);
  t2 = t2 > 1.0f ? 1.0f : t2;
  t2 = t2 < -1.0f ? -1.0f : t2;
  result.y = std::asin(t2);
  // yaw (z-axis rotation)
  F32 t3 = +2.0f * (q.w * q.z + q.x *q.y);
  F32 t4 = +1.0f - 2.0f * (ysqr + q.z * q.z);
  result.z = std::atan2(t3, t4);
  return result;
}

inline Quaternion MatrixToQuaternion(M4& m) {
//https://github.com/g-truc/glm/blob/master/glm/gtc/quaternion.inl
//Adapted from GLM opensource library
  F32 fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
  F32 fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
  F32 fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
  F32 fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

  int biggestIndex = 0;
  F32 fourBiggestSquaredMinus1 = fourWSquaredMinus1;
  if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
  {
    fourBiggestSquaredMinus1 = fourXSquaredMinus1;
    biggestIndex = 1;
  }
  if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
  {
    fourBiggestSquaredMinus1 = fourYSquaredMinus1;
    biggestIndex = 2;
  }
  if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
  {
    fourBiggestSquaredMinus1 = fourZSquaredMinus1;
    biggestIndex = 3;
  }

  F32 biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
  F32 mult = 0.25f / biggestVal;

  Quaternion Result;
  switch (biggestIndex)
  {
  case 0:
    Result.w = biggestVal;
    Result.x = (m[1][2] - m[2][1]) * mult;
    Result.y = (m[2][0] - m[0][2]) * mult;
    Result.z = (m[0][1] - m[1][0]) * mult;
    break;
  case 1:
    Result.w = (m[1][2] - m[2][1]) * mult;
    Result.x = biggestVal;
    Result.y = (m[0][1] + m[1][0]) * mult;
    Result.z = (m[2][0] + m[0][2]) * mult;
    break;
  case 2:
    Result.w = (m[2][0] - m[0][2]) * mult;
    Result.x = (m[0][1] + m[1][0]) * mult;
    Result.y = biggestVal;
    Result.z = (m[1][2] + m[2][1]) * mult;
    break;
  case 3:
    Result.w = (m[0][1] - m[1][0]) * mult;
    Result.x = (m[2][0] + m[0][2]) * mult;
    Result.y = (m[1][2] + m[2][1]) * mult;
    Result.z = biggestVal;
    break;

  default:					// Silence a -Wswitch-default warning in GCC. Should never actually get here. Assert is just for sanity.
    assert(false);
    break;
  }
  return Result;


}






