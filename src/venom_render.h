static const U32 DIRECTIONAL_LIGHT_MAX = 2;
static const U32 POINT_LIGHT_MAX = 64;

namespace UniformLocation
{
	static const U32 model = 0;
	static const U32 view = 1;
	static const U32 projection = 2;
	static const U32 light_space = 3;

	static const U32 cameraViewPosition = 4;
	static const U32 directionalLightCount = 5;
	static const U32 pointLightCount = 6;
	static const U32 directionaLights = 7;
	static const U32 pointLights = directionaLights + DIRECTIONAL_LIGHT_MAX;

};

#pragma pack(push, 1)
struct DirectionalLight
{
	V3 direction;
	V3 color;
};

struct PointLight
{
	V3 position;
	V3 color;
};
#pragma pack(pop)

struct Lighting
{
	DirectionalLight directionalLights[DIRECTIONAL_LIGHT_MAX];
	PointLight pointLights[POINT_LIGHT_MAX];
	int directionalLightCount;
	int pointLightCount;
};