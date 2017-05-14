layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) uniform mat4 uMVPMatrix;

out VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 color;
} vsOut;


void main() {
  //Positions and normals are already in worldSpace
  vsOut.position = inPosition;
  vsOut.normal = inNormal;
  vsOut.color = inColor;

  gl_Position = uMVPMatrix * vec4(inPosition, 1.0);
}