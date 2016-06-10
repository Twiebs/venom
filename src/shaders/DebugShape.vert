layout (location = 0) in vec3 inPosition;

layout (location = 0) uniform mat4 uModelMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uProjectionMatrix;

//TODO(Torin) Lazy hacks
layout (location = 4) uniform vec3 uLineSegmentPositions[2];
layout (location = 6) uniform int uUseLinePos = 0;

void main() {
  if(uUseLinePos == 1) {
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * 
      vec4(uLineSegmentPositions[gl_VertexID], 1.0);
  } else {
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(inPosition, 1.0);
  }
}
