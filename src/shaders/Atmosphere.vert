layout (location = 0) in vec3 inPosition;

layout (location = 0) uniform mat4 uMVPMatrix;
layout (location = 1) uniform vec3 uCameraPosition;

const float RayleighScateringConstant;
const float MieScatteringConstant;
const float SunBrightness;

const int SAMPLE_COUNT = 2;


const float OuterRadius = 5.0f;
const float InnerRadius = 1.0f;
const float Scale = 1.0 / (OuterRadius / InnerRadius);
const float ScaleDepth = 0.25f;
const float ScaleOverScaleDepth = Scale / ScaleDepth;


void main() {
  vec3 cameraToSkydome = inPosition - uCameraPosition;
  float distanceToSkydome = length(cameraToSkydome);
  vec3 direction = cameraToSkydome / distanceToSkydome;

  float height = length(uCameraPosition);

  
  float lengthPerSample = distanceToSkydome / SAMPLE_COUNT;
  vec3 stepPerSample = direction * lengthPerSample;
  vec3 samplePoint = uCameraPosition + (stepPerSample * lengthPerSample);
  vec3 resultColor = vec3(0.0);
  for(int i = 0; i < SAMPLE_COUNT; i++){
    samplePoint += stepPerSample;
  }
}
