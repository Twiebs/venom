struct V2;
struct V3;
struct V4;

struct V2
{
	float x;
	float y;
	V2() {}
	V2(float x, float y) : x(x), y(y) {}
	V2(float s) : x(s), y(s) {}
};

struct V3
{
	union 
	{
		struct 
		{
			float x;
			float y;
			float z;	
		};
		float m[3];
	};
	V3() : x(0), y(0), z(0) {}
	V3(float v) : x(v), y(v), z(v) {}
	V3(float x, float y, float z) : x(x), y(y), z(z) {}
};


struct V4
{
	float x;
	float y;
	float z;
	float w;
	V4() {}
	V4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	V4(float v) : x(v), y(v), z(v), w(v) {}
	V4(const struct V3& v, float w);
};
V4::V4(const V3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}



inline V4 operator-(const V4& a, const V4& b)
{
	V4 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = a.w - b.w;
	return result;
}

inline V3 operator-(const V3& a)
{
	V3 result;
	result.x = -a.x;
	result.y = -a.y;
	result.z = -a.z;
	return result;
}

inline V3 operator+(const V3& a, const V3& b)
{
	V3 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

inline V3 operator-(const V3& a, const V3& b)
{
	V3 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

inline V3 operator*(const V3& a, const V3& b)
{
	V3 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	return result;
}

inline V3 operator/(const V3& a, const V3& b)
{
	V3 result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	return result;
}

inline V3 operator+(const V3& a, const float b)
{
	V3 result;
	result.x = a.x + b;
	result.y = a.y + b;
	result.z = a.z + b;
	return result;
}

inline V3 operator*(const V3& a, const float b)
{
	V3 result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	return result;
}

inline V3& operator+=(V3& a, const V3& b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

inline V3& operator-=(V3& a, const V3&b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}

inline V3& operator*=(V3& a, const V3& b)
{
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	return a;
}

inline V3& operator/=(V3& a, const V3& b)
{
	a.x /= b.x;
	a.y /= b.y;
	a.z /= b.z;
	return a;
}

inline V3& operator+=(V3& a, const float b)
{
	a.x += b;
	a.y += b;
	a.z += b;
	return a;
}

inline V3& operator-=(V3& a, const float b)
{
	a.x -= b;
	a.y -= b;
	a.z -= b;
	return a;
}

inline V3& operator *=(V3& a, const float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
	return a;
}

inline V3& operator /=(V3& a, const float b)
{
	a.x /= b;
	a.y /= b;
	a.z /= b;
	return a;
}

inline V2 operator*=(V2& a, const float b)
{
	a.x *= b;
	a.y *= b;
	return a;
}

inline V2 operator+(const V2& a, const V2& b)
{
	V2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

inline V2 operator-(const V2& a, const V2& b)
{
	V2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

inline V2 operator*(const V2& a, const V2& b)
{
	V2 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return result;
}

inline V2 operator/(const V2& a, const V2& b)
{
	V2 result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	return result;
}

inline float Magnitude(const V2& v)
{
	float result = (v.x*v.x) + (v.y*v.y);
	return result;
}


inline float MagnitudeSquared(const V3& v)
{
	float result = (v.x*v.x) + (v.y*v.y) + (v.z*v.z);
	return result;
}

inline float MagnitudeSquared(const V2& v)
{
	float result = (v.x*v.x) + (v.y*v.y);
	return result;
}


inline float Magnitude(const V3& v)
{
	float result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z *v.z));
	return result;
}


inline V2 Normalize(const V2& v)
{
	float magnitude = Magnitude(v);
	V2 result;
	result.x = v.x / magnitude;
	result.y = v.y / magnitude;
	return result;
}

inline V3 Normalize(const V3& v)
{
	const float magnitude = Magnitude(v);
	V3 result;
	result.x = v.x / magnitude;
	result.y = v.y / magnitude;
	result.z = v.z / magnitude;
	return result;
}

inline V3 Cross(const V3& a, const V3& b)
{
	V3 result;
	result.x = (a.y * b.z) - (a.z * b.y);
	result.y = (a.z * b.x) - (a.x * b.z);
	result.z = (a.x * b.y) - (a.y * b.x);
	return result;
}

inline float Dot(const V3& a, const V3& b)
{
	float result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	return result;
}

