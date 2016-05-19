#include <lighting_fragment.glsl>

in vec2 inFragmentTexcoord;
out vec4 outFragmentColor;

layout (binding = 0) uniform sampler2D uPositionDepthSampler;
layout (binding = 1) uniform sampler2D uNormalSampler;
layout (binding = 2) uniform sampler2D uAlbedoSpecularSampler;

void UnpackGBuffer(ivec2 texcoord, out FragmentInfo fragInfo) {
  vec4 positionDepth = texelFetch(uPositionDepthSampler, texcoord, 0);
  vec4 normal = texelFetch(uNormalSampler, texcoord, 0);
  vec4 albedoSpecular = texelFetch(uAlbedoSpecularSampler, texcoord, 0);
  fragInfo.position = positionDepth.xyz;
  fragInfo.normal = normal.xyz;
  fragInfo.color = albedoSpecular.rgb;
  fragInfo.specularExponent = int(albedoSpecular.a);
}

void main() {
  ivec2 texcoord = ivec2(gl_FragCoord.xy);
  FragmentInfo fragInfo;
  UnpackGBuffer(texcoord, fragInfo);
  outFragmentColor = ApplyLighting(fragInfo);

  //outFragmentColor = vec4(fragInfo.position * 0.5, 1.0);
  //outFragmentColor = vec4((fragInfo.normal * 2.0) - 1.0, 1.0);
  outFragmentColor = vec4((fragInfo.color), 1.0);
  //outFragmentColor = vec4(vec3(texelFetch(uPositionDepthSampler, texcoord, 0).a * 0.05), 1.0);
}