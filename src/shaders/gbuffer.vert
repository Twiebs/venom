layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexcoord;

out VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec2 texcoord;
} vsOut;

layout (location = 0) uniform mat4 uModelMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uProjectionMatrix;

void main() {
  vec4 worldPosition = uModelMatrix * vec4(inPosition, 1.0);
  vsOut.position = worldPosition.xyz;
  vsOut.normal = inNormal;
  vsOut.texcoord = inTexcoord;
  gl_Position = uProjectionMatrix * uViewMatrix * worldPosition;
}
