layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexcoord;

out VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texcoord;
} vsOut;

layout (location = 0) uniform mat4 uModelMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uProjectionMatrix;

void main() {
  //mat4 modelViewMatrix = uViewMatrix * uModelMatrix;
  //vec4 viewspacePosition = modelViewMatrix * vec4(inPosition, 1.0);
  //vec3 viewspaceNormal = mat3(modelViewMatrix) * inNormal;
  //vsOut.position = viewspacePosition.xyz;
  //vsOut.normal = viewspaceNormal.xyz;


  mat4 normalMatrix = transpose(inverse(uModelMatrix));
  vec4 worldspacePosition = uModelMatrix * vec4(inPosition,1.0);
  vsOut.position = worldspacePosition.xyz;
  vsOut.normal = vec3(normalMatrix * vec4(inNormal, 1.0));

  vsOut.tangent = inTangent;
  vsOut.texcoord = inTexcoord;
  gl_Position = uProjectionMatrix * uViewMatrix * worldspacePosition; 
  //gl_Position = uProjectionMatrix * viewspacePosition;
}
