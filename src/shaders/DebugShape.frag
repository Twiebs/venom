layout(location = 3) uniform vec4 inColor;
layout(location = 8) uniform int uIsLightingEnabled;
layout(location = 9) uniform vec3 uLightDirection;
layout(location = 10) uniform vec3 uNormal;

layout (location = 0) out vec3 outColorbuffer;

out vec4 outColor;

void main() {

  vec3 resultColor = vec3(0.0);
  if (uIsLightingEnabled == 1) {
	float nDotD = dot(uNormal, uLightDirection);
	nDotD = clamp(nDotD, 0.0, 1.0);
	resultColor = vec3(inColor) * nDotD;
	resultColor += vec3(inColor) * 0.1;
  } else {
  	resultColor = vec3(inColor);
  }
  

  outColorbuffer = resultColor;
  outColor = vec4(resultColor, 1.0); 
}