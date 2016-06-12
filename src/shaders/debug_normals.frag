out vec4 outColor;

void main() {
  if (gl_Layer == 0) 
    outColor = vec4(1.0, 1.0, 0.0, 1.0);
  if (gl_Layer == 1) 
    outColor = vec4(1.0, 0.0, 1.0, 1.0);

}
