
#include <lighting_vertex.glsl>

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_texcoord;

out VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
} vs_out;

void main()
{
	vec4 world_position = u_model_matrix * vec4(in_position, 1.0f);
	vs_out.position = world_position.xyz;
	vs_out.normal   = in_normal;
	vs_out.texcoord = in_texcoord;
	gl_Position = u_projection_matrix * u_view_matrix * world_position;
}
