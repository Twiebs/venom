layout (location = 0) in vec3 inPosition;

layout (location = 0) uniform mat4 uModelMatrix;

void main() {
  gl_Position = uModelMatrix * vec4(inPosition, 1.0);
}

