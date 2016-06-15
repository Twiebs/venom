
layout (location = 2) uniform vec3 uLightPosition;

in VSOut {
  vec3 rayDirection;
  vec3 scatteringColor;
} fsIn;

out vec4 outColor;

const float MIE_SCATTERING_CONSTANT = -0.75f;
const float MIE_SCATTERING_CONSTANT_SQUARED = MIE_SCATTERING_CONSTANT * MIE_SCATTERING_CONSTANT;

void main(){
  //outColor = vec4(1.0);

  float fCos = dot(uLightPosition, fsIn.rayDirection) / length(fsIn.rayDirection);
  float fMiePhase = 1.5 * ((1.0 - MIE_SCATTERING_CONSTANT_SQUARED) / (2.0 + MIE_SCATTERING_CONSTANT_SQUARED)) * 
    (1.0 + fCos*fCos) / pow(1.0 + MIE_SCATTERING_CONSTANT_SQUARED - 2.0*MIE_SCATTERING_CONSTANT*fCos, 1.5);

  outColor = vec4(fsIn.scatteringColor + fMiePhase, 1.0);
  outColor.a = outColor.b;
}
