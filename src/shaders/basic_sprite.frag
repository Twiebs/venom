in vec2 frag_texcoord;
in vec4 frag_color;

layout (binding = 0) uniform sampler2D sampler;

out vec4 out_color;

void main() {
  out_color = frag_color * texture(sampler, frag_texcoord);
}
