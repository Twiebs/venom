layout (triangles) in;
layout (line_strip, max_vertices = 12) out;

in VS_OUT {
  vec3 normal;
  vec3 tangent;
} gs_in[];

const float MAGNITUDE = 0.1;

void GenerateLineFromVertex(int index, vec3 end) {
  gl_Position = gl_in[index].gl_Position;
  EmitVertex();
  gl_Position = gl_in[index].gl_Position + vec4(end, 0.0f);
  EmitVertex();
  EndPrimitive();
}

void main() {
  gl_Layer = 0;
  GenerateLineFromVertex(0, gs_in[0].normal * MAGNITUDE);
  GenerateLineFromVertex(1, gs_in[1].normal * MAGNITUDE);
  GenerateLineFromVertex(2, gs_in[2].normal * MAGNITUDE);
 
  gl_Layer = 1;
  GenerateLineFromVertex(0, gs_in[0].tangent * MAGNITUDE);
  GenerateLineFromVertex(1, gs_in[1].tangent * MAGNITUDE);
  GenerateLineFromVertex(2, gs_in[2].tangent * MAGNITUDE);
}
