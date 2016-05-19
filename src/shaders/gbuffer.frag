layout (location = 0) out vec4 outPositionDepth;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outAlbedoSpecular;

layout (binding = 0) uniform sampler2D uDiffuseSampler;
layout (binding = 1) uniform sampler2D uNormalSampler;
layout (binding = 2) uniform sampler2D uSpecularSampler;

in VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec2 texcoord;
} fsIn;

const float uProjectionNear = 0.1f;
const float uProjectionFar = 256.0f;

float LinearizeDepth(float depth) {
  float z = depth * 2.0 - 1.0;
  float result = (2.0 * uProjectionNear * uProjectionFar) / 
    (uProjectionFar + uProjectionNear - z * (uProjectionFar - uProjectionNear));
  return result;
}

void main() {
  outPositionDepth.xyz = fsIn.position;
  outPositionDepth.a = LinearizeDepth(gl_FragCoord.z);
  outNormal.xyz = fsIn.normal.xyz;
  outAlbedoSpecular.rgb = texture(uDiffuseSampler, fsIn.texcoord).rgb;
  outAlbedoSpecular.a = texture(uSpecularSampler, fsIn.texcoord).r;
}
