#include <lighting_vertex.glsl>

layout (location = 0) in vec2 in_position;
layout (location = 1) in mat4 in_model_matrix;

layout (binding = 5) uniform sampler2DArray height_sampler;
layout (binding = 6) uniform sampler2DArray normal_sampler;
layout (binding = 7) uniform sampler2DArray u_detail_sampler;

out VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float visibility;
	vec4 light_space_position;
	flat int instance_id;
	float terrain_detail;
} vs_out;

void main() {
	int ix = int(in_position.x);
	int iy = int(in_position.y);
	float terrain_height = texelFetch(height_sampler, ivec3(ix, iy, gl_InstanceID), 0).r;
	float terrain_detail = texelFetch(u_detail_sampler, ivec3(ix, iy, gl_InstanceID), 0).r;
	vec3  terrain_normal = texelFetch(normal_sampler, ivec3(ix, iy, gl_InstanceID), 0).xyz;


	terrain_normal = vec3(0.0, 1.0, 0.0);

	//terrain_height *= TERRAIN_HEIGHT_SCALAR;
	terrain_height = 0.0f;

	vec2 material_texcoord = in_position * (1.0 / 8.0);

	vec4 world_position = in_model_matrix * vec4(in_position.x, terrain_height, in_position.y, 1.0);

	vs_out.position = world_position.xyz;
	vs_out.texcoord = material_texcoord;
	vs_out.normal   = terrain_normal;
	vs_out.visibility = apply_atmospheric_fog(world_position.xyz);
	vs_out.terrain_detail = terrain_detail;
	gl_Position = u_projection_matrix * u_view_matrix * world_position;
}
