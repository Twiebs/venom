layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;

layout (location = 0) uniform mat4 uModelMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uProjectionMatrix;

out VS_OUT {
  vec3 normal;
  vec3 tangent;
} vs_out;

void main() {
  vec4 worldPosition = uModelMatrix * vec4(inPosition, 1.0); 
  mat3 normalMatrix = mat3(transpose(inverse(uViewMatrix * uModelMatrix)));
  
  vs_out.normal = normalize(vec3(uProjectionMatrix * 
    vec4(normalMatrix * inNormal , 1.0)));
  vs_out.tangent = inTangent; 
  gl_Position = uProjectionMatrix * uViewMatrix * worldPosition;
}
