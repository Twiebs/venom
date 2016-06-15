layout (location = 0) in vec3 inPosition;

layout (location = 0) uniform mat4 uMVPMatrix;
layout (location = 1) uniform vec3 uCameraPosition;
layout (location = 2) uniform vec3 uLightPosition;


const int SAMPLE_COUNT = 2;


const float OuterRadius = 5.0f;
const float innerRadius = 1.0f;
const float scale = 1.0 / (OuterRadius / innerRadius);
const float ScaleDepth = 0.25f; 
const float ScaleOverScaleDepth = scale / ScaleDepth;

out VSOut {
  vec3 rayDirection;
  vec3 scatteringColor;
} vsOut;

float Scale(float cosAngle){
  float x = 1.0 - cosAngle;
	float result = ScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
  return result;
}

void main() {
  vec3 cameraToVertex = inPosition - uCameraPosition;
  float distanceToVertex = length(cameraToVertex);
  vec3 directionToVertex = cameraToVertex / distanceToVertex;



  float rayAltitude = uCameraPosition.y;
  float opticalDepth = exp(-4.0 * rayAltitude);
  float startAngle = dot(cameraToVertex, uCameraPosition) / rayAltitude;
  float startOffset = opticalDepth * Scale(startAngle);

  float lengthPerSample = distanceToVertex / SAMPLE_COUNT;
  float scaledLength = lengthPerSample * 0.25f;
  vec3 stepPerSample = directionToVertex * lengthPerSample;
  vec3 samplePoint = uCameraPosition + (stepPerSample * lengthPerSample * 0.5);

  vec3 resultColor = vec3(0.0);
  for(int i = 0; i < SAMPLE_COUNT; i++){
    float altitude = samplePoint.y;
    float opticalDepth = exp(-4.0 * altitude);
    float cameraAngle = dot(cameraToVertex, samplePoint) / altitude;
    float lightAngle = dot(uLightPosition, samplePoint) / altitude;
    float scattering = (startOffset + opticalDepth * (Scale(lightAngle) - Scale(cameraAngle)));
    
    vec3 attenuation = vec3(exp(-scattering * (100000.0)));
    resultColor += (opticalDepth * scaledLength);
    samplePoint += stepPerSample;
  }

  vsOut.rayDirection = uCameraPosition - inPosition;
  vsOut.scatteringColor = resultColor * 0.5f;

  gl_Position = uMVPMatrix * vec4(inPosition, 1.0);
}
