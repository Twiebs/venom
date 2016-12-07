layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexcoord;
layout (location = 4) in ivec4 inBoneIndices;
layout (location = 5) in vec4 inBoneWeights;
layout (location = 6) in int inBoneCount;

out VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texcoord;
} vsOut;

layout (location = 0) uniform mat4 uModelMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uProjectionMatrix;
layout (location = 5) uniform mat4 uBoneOffsets[16];

void main() {
  //mat4 modelViewMatrix = uViewMatrix * uModelMatrix;
  //vec4 viewspacePosition = modelViewMatrix * vec4(inPosition, 1.0);
  //vec3 viewspaceNormal = mat3(modelViewMatrix) * inNormal;
  //vsOut.position = viewspacePosition.xyz;
  //vsOut.normal = viewspaceNormal.xyz;

  vec4 original_vertex = vec4(inPosition, 1.0);
  vec4 original_normal = vec4(inNormal, 0.0);
  vec4 transformed_vertex = vec4(0.0);
  vec4 transformed_normal = vec4(0.0);

#if 0
  for(int i = 0; i < 4; i++) {
    int index = inBoneIndices[i];
    transformed_vertex += (uBoneOffsets[index] * original_vertex) * inBoneWeights[i];
    transformed_normal += (uBoneOffsets[index] * original_normal) * inBoneWeights[i];
  }
#else

  transformed_vertex = original_vertex;
  transformed_normal = original_normal;

#endif




  vec4 worldspacePosition = uModelMatrix * transformed_vertex;
  worldspacePosition.w = 1.0;

  mat4 normalMatrix = transpose(inverse(uModelMatrix));

  vsOut.position = worldspacePosition.xyz;
  vsOut.normal = vec3(normalMatrix * transformed_normal);

  vsOut.tangent = inTangent;
  vsOut.texcoord = inTexcoord;
  gl_Position = uProjectionMatrix * uViewMatrix * worldspacePosition; 
  //gl_Position = uProjectionMatrix * viewspacePosition;
}
