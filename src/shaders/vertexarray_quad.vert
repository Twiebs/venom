out vec3 quad_vertex_position;
out vec2 quad_uv;

out vec2 frag_texcoord;

void main() {
  const vec4 verts[4] = vec4[4](
      vec4(-1.0, -1.0, -1.0, 1.0),
      vec4( 1.0, -1.0, -1.0, 1.0),
      vec4(-1.0,  1.0, -1.0, 1.0),
      vec4( 1.0,  1.0, -1.0, 1.0));
  gl_Position = verts[gl_VertexID];
  quad_vertex_position = vec3(verts[gl_VertexID]);

const vec2 texcoords[4] = vec2[4](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
    vec2(1.0, 1.0));
  frag_texcoord = texcoords[gl_VertexID];

  //quad_vertex_position = vec3(verts[gl_VertexID]);
}
