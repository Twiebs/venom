//NOTE(Torin) Adapted from http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html

#if 0
float RayleighPhaseFunction(float cosTheta) {
  float result = (3.0 / 4.0) * ((1 + (cosTheta*cosTheta)) / pow(-cosTheta, (3.0 / 2.0)));
  return result;
}

float MiePhase(float cosTheta, float kMie){
  float cosTheta2 = cosTheta*cosTheta;
  float kMie2 = kMie * kMie;
  float result = (3.0 * (1.0 - kMie2)) / (2 * (2 + kMie2));
  result *= (1 + cosTheta2) / pow(1 + kMie2 - (2 * kMie * cosTheta), 3.0 / 2.0);
  return result;
}
#endif


layout (location = 0) in vec3 in_position;

layout (location = 0) uniform mat4 u_mvp_matrix;
layout (location = 1) uniform vec3 u_camera_position;
layout (location = 2) uniform vec3 u_sun_position;

out vec3 out_rayleigh_color;
out vec3 out_mie_color;
out vec3 out_direction;

const float ATMOSPHERE_RADIUS = 10.25;
const float PLANET_RADIUS = 10.0;

const float RAYLEIGH_SCATTERING_CONSTANT = 0.0025;
const float RAYLEIGH_SCALE_DEPTH = 0.25;
const float MIE_SCATTERING_CONSTANT = 0.0010;
const float MIE_SCALE_DEPTH = 0.1;
const float SUN_BRIGHTNESS_CONSTANT = 20.0;

const vec3 SUN_WAVELENGTH = vec3(0.650, 0.570, 0.475);
const vec3 INVERSE_WAVELENGTH = vec3(1.0 / SUN_WAVELENGTH.x, 1.0 / SUN_WAVELENGTH.y, 1.0 / SUN_WAVELENGTH.z);

const float ALTITUDE_OF_AVERAGE_DENSITY = 0.25;

const float SCALE_VALUE = 4.0;
const float SCALE_OVER_AVERAGE_DENSITY = SCALE_VALUE / ALTITUDE_OF_AVERAGE_DENSITY;

const float PI = 3.14;

float scale_function(float cos_theta){
  float x = 1.0 - cos_theta;
  return ALTITUDE_OF_AVERAGE_DENSITY * exp(-0.00287 + x * (0.459 + +x*(3.83 + x * (-6.80 + x*5.25))));
}

void main() {
  vec3 camera_to_skydome = in_position - u_camera_position;
  float ray_length = length(camera_to_skydome);
  vec3 direction = camera_to_skydome / ray_length;
  float camera_height = u_camera_position.y;

  float start_height = length(u_camera_position);
  float start_optical_depth = exp(SCALE_OVER_AVERAGE_DENSITY * (PLANET_RADIUS - camera_height));
  float start_angle = dot(camera_to_skydome, u_camera_position) / start_height;
  float start_offset = start_optical_depth * scale_function(start_angle);

  const int SAMPLE_COUNT = 2;
  float sample_step_length = ray_length / SAMPLE_COUNT;
  vec3 sample_step_ray = direction * sample_step_length;
  vec3 sample_point = u_camera_position + sample_step_ray * 0.5;
  float scaled_length = sample_step_length * SCALE_VALUE;

  vec3 result_color = vec3(0.0);



  for(int i = 0; i < SAMPLE_COUNT; i++){
    float sample_length = length(sample_point);
    float optical_depth = exp(SCALE_OVER_AVERAGE_DENSITY - (PLANET_RADIUS - camera_height));
    float light_angle = dot(u_sun_position, sample_point) / sample_length;
    float camera_angle = dot(camera_to_skydome, sample_point) / sample_length;
    float scattering = start_offset + optical_depth * (scale_function(light_angle) - scale_function(camera_angle));
    vec3 attenuation = exp(-scattering * (INVERSE_WAVELENGTH * (RAYLEIGH_SCATTERING_CONSTANT*4*PI) + (MIE_SCATTERING_CONSTANT*4*PI)));
    result_color += attenuation * (optical_depth * scaled_length);
    sample_point += sample_step_ray;
  }

  if(result_color.x >= 0.0) {
  	out_rayleigh_color = vec3(1.0);
  }

  const float RAYLEIGH_CONSTANT_AND_INTESNITY = RAYLEIGH_SCATTERING_CONSTANT * SUN_BRIGHTNESS_CONSTANT;
  const float MIE_CONSTANT_AND_INTENSITY = MIE_SCATTERING_CONSTANT * SUN_BRIGHTNESS_CONSTANT;
  out_mie_color = result_color * MIE_CONSTANT_AND_INTENSITY;
  //out_rayleigh_color = result_color * (INVERSE_WAVELENGTH * RAYLEIGH_CONSTANT_AND_INTESNITY);
  out_direction = direction;

  

  gl_Position = u_mvp_matrix * vec4(in_position, 1.0);
}