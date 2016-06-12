layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(location = 1) uniform mat4 lightTransformMatrices[6];

out vec4 fragPosition;

void main() {
  for (int face = 0; face < 6; face++) {
    gl_Layer = face;
    for (int i = 0; i < 3; i++) {
      fragPosition = gl_in[i].gl_Position;
      gl_Position = lightTransformMatrices[face] * fragPosition;
      EmitVertex();
    }
    EndPrimitive();
  }
}
