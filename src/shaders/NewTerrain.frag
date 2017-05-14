#include <lighting_fragment.glsl>

in VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 color;
} fsIn;

out vec4 outColor;

void main() {
  FragmentInfo fragment;
  fragment.position = fsIn.position;
  fragment.normal = fsIn.normal;
  fragment.color = fsIn.color;
  fragment.specularExponent = 1000;
  outColor = ApplyLighting(fragment);
}
