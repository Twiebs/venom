
in float atmosphereHeight;

out vec4 outColor;

#define RGB8(r, g, b) vec3(float(r) / 255, float(g) / 255, float(b) / 255)

void main() {

  const vec3 lightColor = RGB8(125, 206, 250);
  const vec3 darkColor = RGB8(0, 30, 40);
  outColor.rgb = mix(lightColor, darkColor, atmosphereHeight);
  outColor.a = 1.0;
}
