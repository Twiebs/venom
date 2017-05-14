layout(location = 0) in vec3 inPosition;

layout(location = 0) uniform mat4 uMVPMatrix;
layout(location = 1) uniform vec3 uCameraPosition;
layout(location = 2) uniform vec3 uSunPosition;

out float atmosphereHeight;

void main() {
  atmosphereHeight = inPosition.y;
  gl_Position = uMVPMatrix * vec4(inPosition, 1.0);
}