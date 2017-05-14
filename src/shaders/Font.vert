layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexcoord;

layout(location = 0) uniform mat4 uMVPMatrix;
layout(location = 1) uniform vec4 uColor;

out vec2 fragTexcoord;

void main() {
  fragTexcoord = inTexcoord;
  vec4 position = vec4(inPosition, 1.0);
  gl_Position = uMVPMatrix * position;
}