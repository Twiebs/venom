layout (binding = 0) uniform sampler2D u_diffuse_samplers[TERRAIN_MATERIAL_COUNT];
//layout (binding = 3) uniform sampler2DArray depth_sampler;
layout (binding = 7) uniform sampler2DArray u_detail_sampler;

//TODO(Torin) Make sure that the lighting include does not 
//depend on the depth_sampler
#include <lighting_fragment.glsl>

in VS_OUT {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
	flat int instance_id;
	float terrain_detail;
} fs_in;

out vec4 out_color;

void main()
{
	vec3 diffuse_color_0  = texture(u_diffuse_samplers[0], fs_in.texcoord).rgb;
	vec3 diffuse_color_1  = texture(u_diffuse_samplers[1], fs_in.texcoord).rgb;
	//vec3 fragment_color = mix(diffuse_color_0, diffuse_color_1, fs_in.terrain_detail);
	vec3 fragmentBaseColor = vec3(1.0);

	FragmentInfo fragment;
	fragment.position = fs_in.position;
	fragment.normal = fs_in.normal;
	fragment.specularExponent = 0;
	fragment.color = fragmentBaseColor;

	out_color = ApplyLighting(fragment);
	//out_color.xyz *= CalculateShadowScalar(vec4(fs_in.position, 1.0));
	//out_color = mix(vec4(0.2f, 0.4f, 0.9f, 1.0), out_color, fs_in.visibility);
	//out_color = vec4(fragment.normal, 1.0);
}
