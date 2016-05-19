#include <lighting_vertex.glsl>

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_texcoord;

out VS_OUT {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
  mat3 tbn;
} vs_out;

void main() {
	vec4 world_position = u_model_matrix * vec4(in_position, 1.0f);
	vs_out.position = world_position.xyz;
	vs_out.normal   = in_normal;
	vs_out.texcoord = in_texcoord;
	vs_out.visibility = apply_atmospheric_fog(world_position.xyz);
	vs_out.light_space_position = u_light_space_matrix[0] * world_position;
  vs_out.tbn = CalculateTBNMatrix(in_normal, in_tangent, u_model_matrix);
	gl_Position = u_projection_matrix * u_view_matrix * world_position;
}
