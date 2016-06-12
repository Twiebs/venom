in vec4 fragPosition;

layout(location = 7) uniform vec3 uLightPosition;
layout(location = 8) uniform float uFarPlane;

void main() {
  float lightDistance = length(fragPosition.xyz - uLightPosition);
  lightDistance = lightDistance / uFarPlane;
  gl_FragDepth = lightDistance;
}
