layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;

out VS_OUT {
  vec3 normal;
} vs_out;

void main() {
  mat3 normalMatrix = mat3(transpose(inverse(view * model)));
  vs_out.normal = normalize(vec3(projection * vec4(normalMatrix * normal, 1.0)));
  gl_Position = projection * view * model * vec4(position, 1.0);
}
