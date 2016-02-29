layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;

out vec4 frag_color;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0);
	frag_color = color;
}
