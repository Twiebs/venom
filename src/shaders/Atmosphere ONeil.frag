
layout (location = 2) uniform vec3 u_sun_position;

in vec3 out_rayleigh_color;
in vec3 out_mie_color;
in vec3 out_direction;

out vec4 out_color;

const float MIE_SCATTERING_CONSTANT = -0.75f;
const float MIE_SCATTERING_CONSTANT_SQUARED = MIE_SCATTERING_CONSTANT * MIE_SCATTERING_CONSTANT;

void main() {
  float cos_value = dot(u_sun_position, out_direction) / length(out_direction);
  float fMiePhase = 1.5 * ((1.0 - MIE_SCATTERING_CONSTANT_SQUARED) / (2.0 + MIE_SCATTERING_CONSTANT_SQUARED)) * 
    (1.0 + cos_value*cos_value) / pow(1.0 + MIE_SCATTERING_CONSTANT_SQUARED - 2.0*MIE_SCATTERING_CONSTANT*cos_value, 1.5);


  //outColor = vec4(out_rayleigh_color + out_mie_color * fMiePhase, 1.0);
  //outColor.a = outColor.b;

  out_color = vec4(out_mie_color, 1.0);

  if (out_rayleigh_color.x == 0.0){
    out_color = vec4(1.0, 0.0, 1.0, 1.0);
  } else {
    out_color = vec4(0.7);
  }

  out_color = vec4(out_rayleigh_color, 1.0);

  //out_color = vec4(1.0);

  //outColor = vec4(0.23f, 0.51, 0.65, 1.0);
}
