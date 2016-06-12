const float FOG_DENSITY = 0.005f;
const float FOG_GRADIENT = 2.0f;

#define TERRAIN_CHUNK_PER_EDGE 31
#define TERRAIN_CHUNK_SIZE 31

#define UniformLocation_ModelMatrix 0
#define UniformLocation_ViewMatrix  1
#define UniformLocation_ProjectionMatrix 2
#define UniformLocation_LightSpaceMatrices 3
#define UniformLocation_CameraViewPosition 4

layout (location = UniformLocation_ModelMatrix) uniform mat4 u_model_matrix;
layout (location = UniformLocation_ViewMatrix) uniform mat4 u_view_matrix;
layout (location = UniformLocation_ProjectionMatrix) uniform mat4 u_projection_matrix;
layout (location = UniformLocation_CameraViewPosition) uniform vec3 u_camera_view_position;

uniform mat4 u_light_space_matrix[4];

mat3 CalculateTBNMatrix (vec3 normal, vec3 tangent, mat4 modelMatrix) {
  vec3 bitangent = cross(normalize(tangent), normalize(normal));
  vec3 t = normalize(vec3(modelMatrix * vec4(normalize(tangent),   0.0)));
  vec3 b = normalize(vec3(modelMatrix * vec4(normalize(bitangent), 0.0)));
  vec3 n = normalize(vec3(modelMatrix * vec4(normalize(normal),    0.0)));
  mat3 result = mat3(t, b, n);
  return result;
}

float apply_atmospheric_fog(vec3 position) {
  float fog_end = (TERRAIN_CHUNK_SIZE * 
    float(TERRAIN_CHUNK_PER_EDGE*0.5)) - (TERRAIN_CHUNK_SIZE * 1.25);
	float fog_start = (TERRAIN_CHUNK_SIZE * 
    float(TERRAIN_CHUNK_PER_EDGE*0.5)) - (TERRAIN_CHUNK_SIZE * 5);
	float distance_to_vertex = length(u_camera_view_position - position);

	float fog_factor = clamp((fog_end - distance_to_vertex) / 
    (fog_end - fog_start), 0.0, 1.0);
	return fog_factor;
}
