in vec4 frag_color;

layout (location = 0) out vec3 outColorbuffer;

out vec4 out_color;

void main()
{
	outColorbuffer = vec3(frag_color);
	out_color = frag_color;
}
