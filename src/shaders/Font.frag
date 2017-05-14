layout(location = 1) uniform vec4 uColor;

layout(binding = 0) uniform sampler2D fontAtlas;

in vec2 fragTexcoord;

out vec4 outColor;

void main() {
  outColor = uColor * texture(fontAtlas, fragTexcoord);
  //outColor = vec4(1.0);
}