const int MAX_DIRECTIONAL_LIGHTS = 2;
const int MAX_POINT_LIGHTS = 64;

struct DirectionalLight
{
	vec3 direction;
	vec3 color;
};

struct FragmentInfo
{
  vec3 position;
  vec3 normal;
  vec3 color;
  float specular;
  float depth;
};

layout (location = 4) uniform vec3 u_camera_view_position;
layout (location = 5) uniform int directionalLightCount;
layout (location = 6) uniform int pointLightCount;
layout (location = 7) uniform DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];

uniform mat4 u_light_space_matrix[4];
uniform float u_shadow_cascade_distance[4];

vec3 ApplyDirectionalLight(DirectionalLight light, FragmentInfo fragment, vec3 viewDirection)
{
	float diffuseFactor = max(dot(fragment.normal, light.direction), 0.0);

	vec3 halfwayDirection = normalize(light.direction + viewDirection);
	float specularFactor = pow(max(dot(fragment.normal, halfwayDirection), 0.0), fragment.specular);

	vec3 diffuseColor = light.color * fragment.color * diffuseFactor;
	vec3 specularColor = light.color * fragment.color * specularFactor;
	return diffuseColor + specularColor;
}

vec4 ApplyLighting(FragmentInfo fragment)
{
	vec3 resultColor = vec3(0.0);
	vec3 viewDirection = normalize(u_camera_view_position - fragment.position);
	for (int i = 0; i < directionalLightCount; i++) {
		resultColor += ApplyDirectionalLight(directionalLights[i], fragment, viewDirection);
	}
	return vec4(resultColor, 1.0);
}

float shadow_calaculate(vec4 position)
{
	int index = 0;
	if (gl_FragCoord.z < u_shadow_cascade_distance[3]) index = 3;
	if (gl_FragCoord.z < u_shadow_cascade_distance[2]) index = 2;
	if (gl_FragCoord.z < u_shadow_cascade_distance[1]) index = 1;
	if (gl_FragCoord.z < u_shadow_cascade_distance[0]) index = 0;


	vec4 shadow_coordinate = u_light_space_matrix[index] * position;
	shadow_coordinate = shadow_coordinate * 0.5 + 0.5;
    float closest_depth = texture(depth_sampler, vec3(shadow_coordinate.xy, index)).r;
    float current_depth = shadow_coordinate.z;
    float shadow = current_depth > closest_depth ? 1.0 : 0.2;
    return shadow;
}
