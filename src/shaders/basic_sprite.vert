layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec4 color;

layout (location = 2) uniform mat4 projection;

out vec2 frag_texcoord;
out vec4 frag_color;

void main()
{
	frag_texcoord = texcoord;
	frag_color = color;
	gl_Position = projection * vec4(position, 0, 1);
}
