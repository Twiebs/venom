layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 texCoord;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;

out VertexOut
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
} vs_out;

void main()
{
	mat4 new_view = mat4(mat3(view));

	vs_out.position = vec3(model * vec4(position, 1.0));
	vs_out.normal = normal;
	vs_out.texCoord = texCoord;
	gl_Position = projection * new_view * model * vec4(position, 1.0f);
}
