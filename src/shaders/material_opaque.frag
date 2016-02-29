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
	vec3 diffuse_color = texture(diffuse_sampler, fs_in.texcoord).rgb;

	FragmentInfo fragment;
	fragment.position = fs_in.position;
	fragment.normal = fs_in.normal;
	fragment.color = diffuse_color;
    fragment.specular = 0; //texture(specular_sampler, fs_in.texcoord).r;

	out_color = ApplyLighting(fragment);
    //out_color.rgb *= (1.0 - shadow_calaculate(fs_in.light_space_position));
	out_color = mix(vec4(0.2f, 0.4f, 0.9f, 1.0), out_color, fs_in.visibility);
}
