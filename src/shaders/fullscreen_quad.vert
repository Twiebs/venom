#version 330

const vec4 verts[6] = vec4[6]
(
  vec4(-1.0, -1.0, 0.0, 1.0),
  vec4( 1.0, -1.0, 0.0, 1.0),
  vec4(-1.0,  1.0, 0.0, 1.0),

  vec4( 1.0, -1.0, 0.0, 1.0),
  vec4( 1.0,  1.0, 0.0, 1.0),
  vec4(-1.0,  1.0, 0.0, 1.0)
);

const vec2 texcoords[6] = vec2[6]
(
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),

	vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

out vec2 texcoord;

void main()
{
  texcoord = texcoords[gl_VertexID];
  gl_Position = verts[gl_VertexID];
}