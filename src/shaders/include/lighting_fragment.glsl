struct DirectionalLight {
	vec3 direction;
	vec3 color;
};

struct PointLight {
  vec3 position;
  vec3 color;
  float radius;
};

struct FragmentInfo {
  vec3 position;
  vec3 normal;
  vec3 color;
  int specularExponent;
  float depth;
};

layout (location = 4) uniform vec3 u_camera_view_position;
layout (location = 5) uniform int directionalLightCount;
layout (location = 6) uniform int uPointLightCount;
layout (location = 7) uniform int uShadowCastingPointLightCount;

layout (location = DIRECTIONAL_LIGHT_UNIFORM_LOCATION) uniform DirectionalLight directionalLights[DIRECTIONAL_LIGHTS_MAX];
layout (location = POINT_LIGHT_UNIFORM_LOCATION) uniform PointLight pointLights[POINT_LIGHTS_MAX];
layout (location = SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION) uniform PointLight shadowCastingPointLights[SHADOW_CASTING_POINT_LIGHT_MAX];

layout (binding = 3) uniform sampler2DArray depth_sampler;
layout (binding = 4) uniform samplerCube uOmniShadowMapSampler[SHADOW_CASTING_POINT_LIGHT_MAX];

uniform mat4 u_light_space_matrix[4];
uniform float u_shadow_cascade_distance[4];

vec3 ApplyDirectionalLight(DirectionalLight light, FragmentInfo fragment, vec3 viewDirection) { 
  float diffuseFactor = max(dot(fragment.normal, light.direction), 0.0);
	vec3 halfwayDirection = normalize(light.direction + viewDirection);
	float specularFactor = pow(max(dot(fragment.normal, halfwayDirection), 0.0), fragment.specularExponent);
	vec3 diffuseColor = light.color * fragment.color * diffuseFactor;
	//vec3 specularColor = light.color * fragment.color * specularFactor;
	return diffuseColor;
}

vec3 ApplyPointLight(PointLight light, FragmentInfo fragment, vec3 viewDirection) {
  vec3 lightDisplacement = light.position - fragment.position;
  float lightDistance = length(lightDisplacement);
  vec3 lightDirection = lightDisplacement / lightDistance;
  
  vec3 halfwayDirection = normalize(lightDirection + viewDirection);
  float diffuseFactor = max(dot(lightDirection, fragment.normal), 0.0);
  float specularFactor = pow(max(
    dot(fragment.normal, halfwayDirection), 0.0), fragment.specularExponent);

#if 1
  float attenConstant = 1.0;
  float attenLinear = 0.2;
  //float attenQuadratic = 0.032;
  float attenQuadratic = 0.004;
  float attenuationFactor = 1.0 / (attenConstant + attenLinear * lightDistance +
    attenQuadratic * (lightDistance * lightDistance));
#else
  float cutoffBias = 0.0001;
  float lightIntensity = 1.0f;
  float attenuationFactor = lightIntensity / pow((lightDistance / light.radius) + 1, 2);
  attenuationFactor = (attenuationFactor - cutoffBias) / (1 - cutoffBias);
  attenuationFactor = max(attenuationFactor, 0);
#endif

  vec3 diffuseColor = light.color * fragment.color * diffuseFactor * attenuationFactor;
  vec3 specularColor = light.color * fragment.color * specularFactor * attenuationFactor;
  return diffuseColor + specularColor;
}

float CalcuateOmniShadowMapScalar(vec3 fragPos, vec3 lightPos, in samplerCube sampler) {
  vec3 fragToLight = fragPos - lightPos;
  float shadowmapDepth = texture(sampler, fragToLight).r;
  shadowmapDepth *= 25; 
  float fragmentDepth = length(fragToLight);
  float bias = 0.005;
  float shadow = (fragmentDepth - bias) > shadowmapDepth ? 0.9 : 0.0;
  return shadow;
}

#if 0
float CalculateShadowScalar(vec3 position) {
	int index = 0;
	if (gl_FragCoord.z < u_shadow_cascade_distance[3]) index = 3;
	if (gl_FragCoord.z < u_shadow_cascade_distance[2]) index = 2;
	if (gl_FragCoord.z < u_shadow_cascade_distance[1]) index = 1;
	if (gl_FragCoord.z < u_shadow_cascade_distance[0]) index = 0;

	vec4 shadow_coordinate = u_light_space_matrix[index] * vec4(position, 1.0);
	shadow_coordinate = shadow_coordinate * 0.5 + 0.5;
  float closest_depth = texture(depth_sampler, vec3(shadow_coordinate.xy, index)).r;
  float current_depth = shadow_coordinate.z;
  float bias = 0.0005;
  
  float shadow = 0.0;
  vec2 texelSize = 1.0 / vec2(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION); 
  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      vec3 texcoord = vec3(vec2(shadow_coordinate.xy + vec2(x,y) * texelSize), index);
      float pcfDepth = texture(depth_sampler, texcoord).r;
      shadow += (current_depth - bias) > pcfDepth ? 1.0 : 0.1;
    }
  }
  shadow /= 9.0;
  return shadow;
}
#endif


vec4 ApplyLighting(FragmentInfo fragment) {
  vec3 resultColor = vec3(0.0);
	vec3 viewDirection = normalize(u_camera_view_position - fragment.position);

	for (int i = 0; i < directionalLightCount; i++) {
		vec3 fragmentColor = ApplyDirectionalLight(directionalLights[i], fragment, viewDirection);
    //float resultLighting = CalculateShadowScalar(fragment.position);
    //fragmentColor *= 1.0 - resultLighting;
    resultColor += fragmentColor;
    //resultColor = vec3(1.0, 0.0, 0.0);
	}


#if 0
  for (int i = 0; i < uPointLightCount; i++) {
    vec3 fragmentColor = ApplyPointLight(pointLights[i], fragment, viewDirection);
    resultColor += fragmentColor;
  }
#endif


#if 0
  for (int i = 0; i < uShadowCastingPointLightCount; i++) {
    vec3 fragmentColor = ApplyPointLight(shadowCastingPointLights[i], 
      fragment, viewDirection);
    float resultLighting = CalcuateOmniShadowMapScalar(fragment.position, 
      shadowCastingPointLights[i].position, uOmniShadowMapSampler[i]);
    fragmentColor *= (1.0 - resultLighting) + 0.2;
    resultColor += fragmentColor;
  }
#endif

	return vec4(resultColor, 1.0);
}

