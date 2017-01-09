struct M2 {
	float data[2][2];

	inline float* operator[](size_t i) {
		strict_assert(i >= 0);
		strict_assert(i < 2);
		return (float *)&data[i];
	}
};

struct M3 {
	float data[3][3];
	inline float* operator[](size_t i) {
		strict_assert(i >= 0);
		strict_assert(i < 3);
		return (float *)&data[i];
	}
};



inline M4 operator*(const M4& a, const M4& b) {
	M4 result = {};
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			for (int i = 0; i < 4; i++) {
				result[c][r] += a[i][r] * b[c][i];
			}
		}
	}
	return result;
}

inline V4 operator*(const M4& a, const V4& b)
{
	V4 result;
	float *r = &result.x;
	for (int i = 0; i < 4; i++)
			 r[i] = (a[0][i]*b.x) + (a[1][i]*b.y)
				+ (a[2][i]*b.z) + (a[3][i]*b.w);
	return result;
}

inline M4 operator*(const M4& a, const F32 b) {
  M4 result = {};
  for (size_t y = 0; y < 4; y++) {
    for (size_t x = 0; x < 4; x++) {
      result[y][x] = a[y][x] * b;
    }
  }
  return result;
}

inline M4 operator+(const M4& a, const M4& b) {
  M4 result = {};
  for (size_t y = 0; y < 4; y++) {
    for (size_t x = 0; x < 4; x++) {
      result[y][x] = a[y][x] + b[y][x];
    }
  }
  return result;
}

inline M4 M4Zero()
{
	M4 result = {};
	return result;
}

inline bool Equals(M4& a, M4& b){
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      if (!Equals(a[i][j], b[i][j])) return false;
    }
  }
  return true;
}

inline M4 M4Identity()
{

	M4 m;
	m[0][0] = 1.0f; m[1][0] = 0.0f; m[2][0] = 0.0f; m[3][0] = 0.0f;
	m[0][1] = 0.0f; m[1][1] = 1.0f; m[2][1] = 0.0f; m[3][1] = 0.0f;
	m[0][2] = 0.0f; m[1][2] = 0.0f; m[2][2] = 1.0f; m[3][2] = 0.0f;
	m[0][3] = 0.0f; m[1][3] = 0.0f; m[2][3] = 0.0f; m[3][3] = 1.0f;
	return m;
}

#if 0
inline M4 Inverse(const M4& a)
{
	float determinate = 
		a[0][0]*a[1][1]*a[2][2]*a[3][3] +
		a[0][0]*a[1][2]*a[2][3]*a[3][1] +
		a[0][0]*a[1][3]*a[2][1]*a[3][2] +

		a[0][1]*a[1][0]*a[2][3]*a[3][2] +
		a[0][1]*a[1][2]*a[2][0]*a[3][3] +
		a[0][1]*a[1][3]*a[2][2]*a[3][0] +

		a[0][2]*a[1][0]*a[2][3]*a[3][2] +
		a[0][2]*a[1][1]*a[2][3]*a[3][0] +
		a[0][2]*a[1][3]*a[2][0]*a[3][1] +

		a[0][3]*a[1][0]*a[2][2]*a[3][1] +
		a[0][3]*a[1][1]*a[2][0]*a[3][2] +
		a[0][3]*a[1][2]*a[2][1]*a[3][0] -
}
#endif

inline M4 Inverse(const M4& a)
{
	const float *m = &a[0][0];
	float inv[16], determinate;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    determinate = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (determinate == 0)
		return M4Identity();

    determinate = 1.0f / determinate;

	M4 result;
	float *r = &result[0][0];
    for (i = 0; i < 16; i++)
        r[i] = inv[i] * determinate;

	return result;
}


inline M4 Transpose(const M4& m)
{
	M4 result;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			result[i][j] = m[j][i];
	return result;
}



inline M4 Translate(float x, float y, float z)
{
	M4 m;
	m[0][0] = 1.0f; m[1][0] = 0.0f; m[2][0] = 0.0f; m[3][0] = x;
	m[0][1] = 0.0f; m[1][1] = 1.0f; m[2][1] = 0.0f; m[3][1] = y;
	m[0][2] = 0.0f; m[1][2] = 0.0f; m[2][2] = 1.0f; m[3][2] = z;
	m[0][3] = 0.0f; m[1][3] = 0.0f; m[2][3] = 0.0f; m[3][3] = 1.0f;
	return m;
}

#if 1
inline M4 Scale(float x, float y, float z)
{
	M4 m;
	m[0][0] = x;    m[1][0] = 0.0f; m[2][0] = 0.0f; m[3][0] = 0.0f;
	m[0][1] = 0.0f; m[1][1] = y;    m[2][1] = 0.0f; m[3][1] = 0.0f;
	m[0][2] = 0.0f; m[1][2] = 0.0f; m[2][2] = z;    m[3][2] = 0.0f;
	m[0][3] = 0.0f; m[1][3] = 0.0f; m[2][3] = 0.0f; m[3][3] = 1.0f;
	return m;
}
#endif

inline M4 Rotate(float x, float y, float z) {
	M4 rx, ry, rz;

	rx[0][0] = 1.0f; rx[1][0] =  0.0f;		rx[2][0] = 0.0f; 		  rx[3][0] = 0.0f;
	rx[0][1] = 0.0f; rx[1][1] =  cosf(x); rx[2][1] = sinf(x); 	rx[3][1] = 0.0f;
	rx[0][2] = 0.0f; rx[1][2] = -sinf(x);	rx[2][2] = cosf(x);   rx[3][2] = 0.0f;
	rx[0][3] = 0.0f; rx[1][3] =  0.0f;		rx[2][3] = 0.0f; 		  rx[3][3] = 1.0f;

	ry[0][0] =  cosf(y); 	ry[1][0] = 0.0f;	ry[2][0] = sinf(y); 	ry[3][0] = 0.0f;
	ry[0][1] =  0.0f; 		ry[1][1] = 1.0f;  ry[2][1] = 0.0f; 		  ry[3][1] = 0.0f;
	ry[0][2] = -sinf(y); 	ry[1][2] = 0.0f;	ry[2][2] = cosf(y);   ry[3][2] = 0.0f;
	ry[0][3] =  0.0f; 		ry[1][3] = 0.0f;	ry[2][3] = 0.0f; 		  ry[3][3] = 1.0f;

	rz[0][0] =  cosf(z); 	rz[1][0] = sinf(z);		rz[2][0] = 0.0f; 	rz[3][0] = 0.0f;
	rz[0][1] = -sinf(z); 	rz[1][1] = cosf(z);   rz[2][1] = 0.0f; 	rz[3][1] = 0.0f;
	rz[0][2] =  0.0f; 		rz[1][2] = 0.0f;		  rz[2][2] = 1.0f;  rz[3][2] = 0.0f;
	rz[0][3] =  0.0f; 		rz[1][3] = 0.0f;		  rz[2][3] = 0.0f; 	rz[3][3] = 1.0f;

	M4 result = rz * ry * rx;
	return result;
}

inline M4 Translate(const V3& v) { return Translate(v.x, v.y, v.z); }
inline M4 Rotate(const V3& v) { return Rotate(v.x, v.y, v.z); }
inline M4 Scale(const V3 v) { return Scale(v.x, v.y, v.z); }
inline M4 Scale(const float s) { return Scale(s, s, s); }

inline M4 Orthographic(float left, float right, float bottom, float top,
	float near_clip, float far_clip, float up)
{
	float x = 2.0f / (right - left);
	float y = 2.0f / (top - bottom);
	float z = -2.0f / (far_clip - near_clip);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	float tz = -(far_clip + near_clip) / (far_clip - near_clip);

	y *= up;
	ty *= up;

	M4 r;
	r[0][0] = x;		r[1][0] = 0.0f;		r[2][0] = 0.0f;		r[3][0] = tx;
	r[0][1] = 0.0f;		r[1][1] = y;		r[2][1] = 0.0f;		r[3][1] = ty;
	r[0][2] = 0.0f;		r[1][2] = 0.0f;		r[2][2] = z;		r[3][2] = tz;
	r[0][3] = 0.0f;		r[1][3] = 0.0f;		r[2][3] = 0.0f;		r[3][3] = 1.0f;
	return r;
}

inline M4 Perspective(float fieldOfView, float viewportWidth, float viewportHeight, float near_clip, float far_clip) 
{
	const float ar = viewportWidth / viewportHeight;
	const float tanHalfFOV = tanf(fieldOfView * 0.5f);

	const float r00 = 1.0f / (ar * tanHalfFOV);
	const float r11 = 1.0f / (tanHalfFOV);
	const float r22 = -(far_clip + near_clip) / (far_clip - near_clip);
	const float r32 = -(2 * far_clip * near_clip) / (far_clip - near_clip);

	M4 m;

	m[0][0] = r00;   m[1][0] = 0.0f;  m[2][0] = 0.0f;   m[3][0] = 0.0f;
	m[0][1] = 0.0f;  m[1][1] = r11;   m[2][1] = 0.0f;   m[3][1] = 0.0f;
	m[0][2] = 0.0f;  m[1][2] = 0.0f;  m[2][2] = r22;    m[3][2] = r32;
	m[0][3] = 0.0f;  m[1][3] = 0.0f;  m[2][3] = -1.0f;  m[3][3] = 0.0f;
	return m;
}

inline M4 LookAt(const V3& position, const V3& target, const V3& up) 
{
	const V3 zaxis = Normalize(target - position);
	const V3 xaxis = Normalize(Cross(zaxis, up));
	const V3 yaxis = Cross(xaxis, zaxis);

	M4 m;
	m[0][0] = xaxis.x;  m[1][0] = xaxis.y;  m[2][0] = xaxis.z;  m[3][0] = -Dot(xaxis, position);
	m[0][1] = yaxis.x;  m[1][1] = yaxis.y;  m[2][1] = yaxis.z;  m[3][1] = -Dot(yaxis, position); 
	m[0][2] = -zaxis.x; m[1][2] = -zaxis.y; m[2][2] = -zaxis.z; m[3][2] =  Dot(zaxis, position);
	m[0][3] = 0.0f;     m[1][3] = 0.0f;     m[2][3] = 0.0f;     m[3][3] =  1.0f;

	return m;
}

inline M4 DirectionToRotationMatrix(V3 direction) {
  const V3 zaxis = Normalize(direction);
  const V3 xaxis = Normalize(Cross(zaxis, V3(0.0f, 1.0f, 0.0f)));
  const V3 yaxis = Normalize(Cross(xaxis, zaxis));

  M4 m;
  m[0][0] = xaxis.x;  m[1][0] = xaxis.y;  m[2][0] = xaxis.z;  m[3][0] = 0.0f;
  m[0][1] = yaxis.x;  m[1][1] = yaxis.y;  m[2][1] = yaxis.z;  m[3][1] = 0.0f;
  m[0][2] = -zaxis.x; m[1][2] = -zaxis.y; m[2][2] = -zaxis.z; m[3][2] = 0.0f;
  m[0][3] = 0.0f;     m[1][3] = 0.0f;     m[2][3] = 0.0f;     m[3][3] = 1.0f;
  return m;
}

inline Transform DecomposeTransformationMatrix(M4 m) {
  Transform result = {};
  result.translation = V3(m[3][0], m[3][1], m[3][2]);
  float sx = Magnitude(V3(m[0][0], m[0][1], m[0][2]));
  float sy = Magnitude(V3(m[1][0], m[1][1], m[1][2]));
  float sz = Magnitude(V3(m[2][0], m[2][1], m[2][2]));
  result.scale = V3(sx, sy, sz);
  m[0][0] /= sx; m[1][0] /= sy; m[2][0] /= sz;
  m[0][1] /= sx; m[1][1] /= sy; m[2][1] /= sz;
  m[0][2] /= sx; m[1][2] /= sy; m[2][2] /= sz;
  m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f;
  result.rotation = MatrixToQuaternion(m);
  return result;
}