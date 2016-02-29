
#include <lighting_vertex.glsl>

out VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
	int instance_id;
} vs_out;

void main()
{
	vec4 world_position = u_model_matrix * vec4(in_position, 1.0);
	vs_out.position = world_position.xyz;
	vs_out.normal 	= in_normal;
	vs_out.texcoord = world_position.xz * 0.2;
	vs_out.visibility = apply_atmospheric_fog(world_position.xyz);
	vs_out.light_space_position = u_light_space_matrix[0] * world_position;
	gl_Position = u_projection_matrix * u_view_matrix * world_position;
}
