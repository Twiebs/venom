layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;

out vec2 fragmentTexcoord;

void main() {
  const vec4 verts[4] = vec4[4](
      vec4(-1.0, -1.0, 0.5, 1.0),
      vec4( 1.0, -1.0, 0.5, 1.0),
      vec4(-1.0,  1.0, 0.5, 1.0),
      vec4( 1.0,  1.0, 0.5, 1.0));
  gl_Position = verts[gl_VertexID];
}
