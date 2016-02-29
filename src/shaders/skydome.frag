in VertexOut
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
} fs_in;

out vec4 out_color;

void main()
{
	float interp_value = fs_in.position.y / 1.0f;

	const vec4 blend_from = vec4(0.2f, 0.4f, 0.9f, 1.0);
	const vec4 blend_to = vec4(0.0f, 0.2, 0.6f, 1.0);
	//vec4 result = mix(blend_from, blend_to, interp_value);
	vec4 result = blend_from;

	out_color = result;
}
