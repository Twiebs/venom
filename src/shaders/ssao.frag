layout (binding = 0) uniform sampler2D uPositionDepthSampler;
layout (binding = 1) uniform sampler2D uNormalSampler;
layout (binding = 2) uniform sampler2D uNoiseSampler;

layout (location = 0) uniform mat4 uViewMatrix;
layout (location = 1) uniform mat4 uProjectionMatrix;
layout (location = 2) uniform vec3 uSamples[SSAO_SAMPLE_COUNT];

layout (location = 0) out float outOcclusion;

void main() {
  vec2 texcoord = vec2(gl_FragCoord.xy) / textureSize(uPositionDepthSampler, 0);
  vec4 fragPositionDepth = texture(uPositionDepthSampler, texcoord);
  vec3 fragNormal = texture(uNormalSampler, texcoord).xyz;
  vec3 fragPosition = fragPositionDepth.xyz;
  float fragDepth = fragPositionDepth.a;
  //fragNormal = normalize(fragNormal);
  //fragNormal = vec3(uViewMatrix * vec4(fragNormal, 1.0));

  int randomVectorIndex = (int(gl_FragCoord.x * 7123.2315 + 125.232) *
     int(gl_FragCoord.y * 3137.1519 + 234.8)) ^ int(fragDepth) % 64;
  vec3 randomVector = uSamples[randomVectorIndex];

  //vec2 noiseScale = vec2(1600.0 / 4.0, 900.0 / 4.0);
  //vec3 randomVector = texture(uNoiseSampler, texcoord * noiseScale).xyz;

  float distancePerStep = (randomVector.x + 3.0) * 0.1;
  //float distancePerStep = 0.5; 

  const int stepCount = 4;
  float result = 0.0;
  float total = 0.0;
  for (int i = 0; i < SSAO_SAMPLE_COUNT; i++) {
    vec3 direction = uSamples[i];
    direction = (dot(fragNormal, direction) < 0.0) ? -direction : direction;
    float currentStepDistance = 0.0;
    float targetStepDepth = fragDepth; 

    total += 4.0;
    for (int stepIndex = 0; stepIndex < stepCount; stepIndex++) {
      currentStepDistance += distancePerStep;
      targetStepDepth -= direction.z * currentStepDistance; 
      float currentStepDepth = texture(uPositionDepthSampler,
        texcoord + direction.xy * currentStepDistance).a;

      float occlusionWeight = abs(currentStepDepth - fragDepth);
      occlusionWeight *= occlusionWeight;

      if (targetStepDepth - currentStepDepth > 0.0) {
        result += 4.0 / (1.0 + occlusionWeight);
      }
    }
  }

  result = 1.0 - (result / total);
  //result = result / total;
  outOcclusion = result; 
}
