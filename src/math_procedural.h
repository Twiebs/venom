



struct RNGSeed
{
	const U64 seed;
	RNGSeed(U64 seed) : seed(seed) {}
};

float randf01(const RNGSeed& seed)
{
	//TODO(Torin) Make this an Xor random 
	//or change the design of the rng and use mersen-twister
	float result = rand() / RAND_MAX + seed.seed;
	return result;
}

float RandomInRange(const RNGSeed& seed, const float min, const float max)
{
	float result = (randf01(seed)*(max - min)) + min;
	return result;
}

inline V4 RandomSolidColor(const RNGSeed& seed)
{
	V4 result;
	result.x = RandomInRange(seed, 0.0f, 1.0f);
	result.y = RandomInRange(seed, 0.0f, 1.0f);
	result.z = RandomInRange(seed, 0.0f, 1.0f);
	result.w = 1.0f;
	return result;
}

inline V3 RandomInRange(const RNGSeed& seed, const V3& min, const V3& max)
{
	V3 result;
	result.x = RandomInRange(seed, min.x, max.x);
	result.y = RandomInRange(seed, min.y, max.y);
	result.z = RandomInRange(seed, min.z, max.z);
	return result;
}

