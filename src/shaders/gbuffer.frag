layout(location = 0) out vec4 outPositionDepth;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outAlbedoSpecular;

in VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texcoord;
} fsIn;

layout (binding = 0) uniform sampler2D uDiffuseSampler;
layout (binding = 1) uniform sampler2D uNormalSampler;
layout (binding = 2) uniform sampler2D uSpecularSampler;

layout(location = 3) uniform int isNormalmapPresent;
layout(location = 4) uniform int isSpecularmapPresent;

const float uProjectionNear = 0.1f;
const float uProjectionFar = 256.0f;

float LinearizeDepth(float depth) {
  float z = depth * 2.0 - 1.0;
  float result = (2.0 * uProjectionNear * uProjectionFar) / 
    (uProjectionFar + uProjectionNear - z * (uProjectionFar - uProjectionNear));
  return result;
}

void main() {
  vec3 normal = normalize(fsIn.normal);
  if(isNormalmapPresent == 1){ //For Debugging
    vec3 fragmentNormal = normal; 
    vec3 fragmentTangent = normalize(fsIn.tangent);
    vec3 bitangent = cross(fragmentNormal, fragmentTangent);
    mat3 tbn = mat3(fsIn.tangent, bitangent, fsIn.normal);
    normal = texture(uNormalSampler, fsIn.texcoord).xyz;
    normal = normalize((normal * 2.0) - 1.0);
    normal = tbn * normal; 
  }

  outPositionDepth.xyz = fsIn.position;
  outPositionDepth.a = fsIn.position.z;
  outNormal.xyz = normal; 
  //outNormal.xyz = normalize(fsIn.normal); 
  outAlbedoSpecular.rgb = texture(uDiffuseSampler, fsIn.texcoord).rgb;
  outAlbedoSpecular.a = texture(uSpecularSampler, fsIn.texcoord).r * 256;
}
