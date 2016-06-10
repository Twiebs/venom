layout (binding = 0) uniform sampler2D diffuse_sampler;
layout (binding = 1) uniform sampler2D normal_sampler;
layout (binding = 2) uniform sampler2D specular_sampler;

//layout (binding = 3) uniform sampler2DArray depth_sampler;
//layout (binding = 4) 
//uniform samplerCube uOmniShadowMapSampler[SHADOW_CASTING_POINT_LIGHT_MAX];

in VS_OUT {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
  mat3 tbn;
} fs_in;

#include <lighting_fragment.glsl>

out vec4 out_color;

void main() {
	vec3 diffuse_color = texture(diffuse_sampler, fs_in.texcoord).rgb;
  vec3 normal = texture(normal_sampler, fs_in.texcoord).xyz;
  normal = (normal * 2.0) - 1.0;
  normal = normalize(fs_in.tbn * normal);

  vec3 specularSample = texture(specular_sampler, fs_in.texcoord).rgb;
  float specularScalar = specularSample.r + specularSample.g + specularSample.b;
  specularScalar /= 3.0;

	FragmentInfo fragment;
	fragment.position = fs_in.position;
	fragment.normal = fs_in.normal;
  fragment.color = vec3(1.0);
  fragment.specularExponent = 1;

  //fragment.normal = normal; 

	//fragment.color = diffuse_color;
  //fragment.specularExponent = int(specularScalar * 512); 
  //fragment.specularExponent = 1;

  out_color = ApplyLighting(fragment);
  out_color = vec4(diffuse_color, 1.0);
  //out_color = vec4(vec3(specularScalar), 1.0);
  //out_color = vec4(normal, 1.0);

  
  //out_color = mix(vec4(0.2f, 0.4f, 0.9f, 1.0), out_color, fs_in.visibility);

#if 0
	if (gl_FragCoord.z < u_shadow_cascade_distance[3])
		out_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
	if (gl_FragCoord.z < u_shadow_cascade_distance[2])
		out_color = vec4(0.0f, 1.0f, 1.0f, 1.0f);
	if (gl_FragCoord.z < u_shadow_cascade_distance[1])
		out_color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
if (gl_FragCoord.z < u_shadow_cascade_distance[0])
		out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
#endif
}
