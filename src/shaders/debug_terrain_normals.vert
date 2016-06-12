layout (location = 0) in vec3 inPosition;
layout (location = 1) in mat4 inModelMatrix;

layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uProjectionMatrix;

layout (binding = 5) uniform sampler2DArray heightSampler;
layout (binding = 6) uniform sampler2DArray normalSampler;


out VS_OUT {
  vec3 normal;
} vs_out;

void main() {
	int ix = int(inPosition.x);
	int iy = int(inPosition.y);
  float terrainHeight = texelFetch(heightSampler, ivec3(ix, iy, gl_InstanceID), 0).r;
  terrainHeight *= TERRAIN_HEIGHT_SCALAR;
  vec3 terrainNormal = texelFetch(normalSampler, ivec3(ix, iy, gl_InstanceID), 0).xyz;
  vec4 worldPosition = inModelMatrix * 
    vec4(inPosition.x, terrainHeight, inPosition.y, 1.0);

  mat3 normalMatrix = mat3(transpose(inverse(uViewMatrix * inModelMatrix)));
  vs_out.normal = normalize(vec3(uProjectionMatrix * 
    vec4(normalMatrix * terrainNormal, 1.0)));
  gl_Position = uProjectionMatrix  * uViewMatrix * worldPosition;
}
