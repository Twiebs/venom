struct V2;
struct V3;
struct V4;

struct V2 {
	float x, y;
	V2() {}
	V2(float x, float y) : x(x), y(y) {}
	explicit V2(float s) : x(s), y(s) {}
};

struct V3 {
  union { 
    struct { float x, y, z; };
    struct { float r, g, b; };
    float e[3];
  };

	V3() : x(0), y(0), z(0) {}
	V3(float v) : x(v), y(v), z(v) {}
	V3(float x, float y, float z) : x(x), y(y), z(z) {}
  V3(const V3& v) : x(v.x), y(v.y), z(v.z) {}
  V3(const V4& v);
  inline float& operator[](const size_t index) { return e[index]; }
  inline float operator[](const size_t index) const { return e[index]; }
};

struct V4 {
  float x, y, z, w;
	V4() {}
	V4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	V4(float v) : x(v), y(v), z(v), w(v) {}
	V4(const struct V3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};

V3::V3(const V4& v){
  this->x = v.x;
  this->y = v.y;
  this->z = v.z;
}

inline V4 operator-(const V4& a, const V4& b)  {
	V4 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = a.w - b.w;
	return result;
}

inline V3 operator-(const V3& a) {
	V3 result;
	result.x = -a.x;
	result.y = -a.y;
	result.z = -a.z;
	return result;
}

inline V3 operator+(const V3& a, const V3& b) {
	V3 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

inline V3 operator-(const V3& a, const V3& b) {
	V3 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

inline V3 operator*(const V3& a, const V3& b) {
	V3 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	return result;
}

inline V3 operator/(const V3& a, const V3& b) {
	V3 result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	return result;
}

inline V3 operator+(const V3& a, const float b) {
	V3 result;
	result.x = a.x + b;
	result.y = a.y + b;
	result.z = a.z + b;
	return result;
}

inline V3 operator*(const V3& a, const float b) {
	V3 result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	return result;
}

inline V3 operator*(const float b, const V3& a) {
	V3 result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	return result;
}

inline V3& operator+=(V3& a, const V3& b) {
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

inline V3& operator-=(V3& a, const V3&b) {
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}

inline V3& operator*=(V3& a, const V3& b) {
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	return a;
}

inline V3& operator/=(V3& a, const V3& b) {
	a.x /= b.x;
	a.y /= b.y;
	a.z /= b.z;
	return a;
}

inline V3& operator+=(V3& a, const float b) {
	a.x += b;
	a.y += b;
	a.z += b;
	return a;
}

inline V3& operator-=(V3& a, const float b) {
	a.x -= b;
	a.y -= b;
	a.z -= b;
	return a;
}

inline V3& operator *=(V3& a, const float b) {
	a.x *= b;
	a.y *= b;
	a.z *= b;
	return a;
}

inline V3& operator /=(V3& a, const float b) {
	a.x /= b;
	a.y /= b;
	a.z /= b;
	return a;
}

inline V2 operator*=(V2& a, const float b) {
	a.x *= b;
	a.y *= b;
	return a;
}

inline V2 operator+(const V2& a, const V2& b) {
	V2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

inline V2 operator-(const V2& a, const V2& b) {
	V2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

inline V2 operator*(const V2& a, const V2& b) {
	V2 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return result;
}

inline V2 operator/(const V2& a, const V2& b) {
	V2 result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	return result;
}

inline float Magnitude(const V2& v) {
	float result = (v.x*v.x) + (v.y*v.y);
	return result;
}

inline float MagnitudeSquared(const V3& v) {
	float result = (v.x*v.x) + (v.y*v.y) + (v.z*v.z);
	return result;
}

inline float MagnitudeSquared(const V2& v) {
	float result = (v.x*v.x) + (v.y*v.y);
	return result;
}

inline float Magnitude(const V3& v) {
	float result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z *v.z));
	return result;
}

inline float Magnitude(const V4& v) {
	float result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z *v.z) + (v.w * v.w));
	return result;
}

inline V3 Abs(const V3& v) {
  V3 result = {};
  result.x = abs(v.x);
  result.y = abs(v.y);
  result.z = abs(v.z);
  return result;
}

inline V2 Normalize(const V2& v) {
	float magnitude = Magnitude(v);
	V2 result;
	result.x = v.x / magnitude;
	result.y = v.y / magnitude;
	return result;
}

inline V3 Normalize(const V3& v) {
	const float magnitude = Magnitude(v);
	V3 result;
	result.x = v.x / magnitude;
	result.y = v.y / magnitude;
	result.z = v.z / magnitude;
	return result;
}

inline V4 Normalize(const V4& v) {
	const float magnitude = Magnitude(v);
	V4 result;
	result.x = v.x / magnitude;
	result.y = v.y / magnitude;
	result.z = v.z / magnitude;
  result.w = v.w / magnitude;
	return result;
}

inline V3 Cross(const V3& a, const V3& b) {
	V3 result;
	result.x = (a.y * b.z) - (a.z * b.y);
	result.y = (a.z * b.x) - (a.x * b.z);
	result.z = (a.x * b.y) - (a.y * b.x);
	return result;
}

inline float Dot(const V3& a, const V3& b) {
	float result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	return result;
}

inline B32 Equals(const V3& a, const V3& b, const F32 epsilon = 0.001f) {
	if (std::abs(b.x - a.x) > epsilon) return false;
	if (std::abs(b.y - a.y) > epsilon) return false;
	if (std::abs(b.z - a.z) > epsilon) return false;
	return true;
}

inline B32 Equals(const V4& a, const V4& b, const F32 epsilon) {
	if (std::abs(b.x - a.x) > epsilon) return false;
	if (std::abs(b.y - a.y) > epsilon) return false;
	if (std::abs(b.z - a.z) > epsilon) return false;
	if (std::abs(b.w - a.w) > epsilon) return false;
	return true;
}

inline V3 Lerp(V3 a, V3 b, F32 t) {
  V3 result;
  result.x = a.x + t * (b.x - a.x);
  result.y = a.y + t * (b.y - a.y);
  result.z = a.z + t * (b.z - a.z);
  return result;
}

inline V3 Project(const V3& a, const V3& b) {
  F32 dot = Dot(a, b);
  V3 result = b * (dot / MagnitudeSquared(b));
  return result;
}

F32 Orient2D(V2 a, V2 b, V2 c) {
  return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
}

inline F32 TrippleProduct(const V3& a, const V3& b, const V3& c) {
  F32 result = Dot(a, Cross(b, c));
  return result;
}
