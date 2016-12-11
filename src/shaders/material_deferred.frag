#include <lighting_fragment.glsl>

in vec2 inFragmentTexcoord;
out vec4 outFragmentColor;

layout (binding = 0) uniform sampler2D uPositionDepthSampler;
layout (binding = 1) uniform sampler2D uNormalSampler;
layout (binding = 2) uniform sampler2D uAlbedoSpecularSampler;
layout (binding = 3) uniform sampler2D uSSAOSampler;

void UnpackGBuffer(ivec2 texcoord, out FragmentInfo fragInfo) {
  vec4 positionDepth = texelFetch(uPositionDepthSampler, texcoord, 0);
  vec4 normal = texelFetch(uNormalSampler, texcoord, 0);
  vec4 albedoSpecular = texelFetch(uAlbedoSpecularSampler, texcoord, 0);
  fragInfo.position = positionDepth.xyz;
  fragInfo.normal = normalize(normal.xyz);
  fragInfo.color = albedoSpecular.rgb;
  fragInfo.specularExponent = int(albedoSpecular.a);
}

void main() {
  ivec2 texcoord = ivec2(gl_FragCoord.xy);
  FragmentInfo fragInfo;
  UnpackGBuffer(texcoord, fragInfo);
  outFragmentColor = ApplyLighting(fragInfo);
  
  //outFragmentColor = vec4(fragInfo.normal, 1.0);

  //float depth = texelFetch(uPositionDepthSampler, texcoord, 0).r;
  //outFragmentColor = vec4(vec3(depth * 0.1), 1.0);
  //outFragmentColor = vec4((fragInfo.position), 1.0);
  //outFragmentColor = vec4((fragInfo.normal + 1.0) * 0.5, 1.0);
  //outFragmentColor = vec4((fragInfo.color), 1.0);
  //outFragmentColor = vec4(vec3(fragInfo.specularExponent) / 256.0, 1.0);

  //outFragmentColor = vec4(texelFetch(uSSAOSampler, texcoord, 0).r);
}
