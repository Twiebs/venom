layout (location = 0) in vec3 in_position;

layout (location = 0) uniform mat4 uModelMatrix;
layout (location = 3) uniform mat4 uLightSpaceMatrix;

void main() {
	gl_Position = uLightSpaceMatrix * uModelMatrix * vec4(in_position, 1.0f);
}
