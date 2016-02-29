layout (binding = 0) uniform sampler2D diffuse_sampler;
layout (binding = 1) uniform sampler2D normal_sampler;
layout (binding = 2) uniform sampler2D specular_sampler;
layout (binding = 3) uniform sampler2DArray depth_sampler;

in VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
} fs_in;

#include <lighting_fragment.glsl>



out vec4 out_color;

void main()
{
	vec4 diffuse_color = texture(diffuse_sampler, fs_in.texcoord);

	FragmentInfo fragment;
	fragment.position = fs_in.position;
	fragment.normal = fs_in.normal;
	fragment.color = diffuse_color.rgb;

	out_color = vec4(ApplyLighting(fragment).rgb, diffuse_color.a);
}
