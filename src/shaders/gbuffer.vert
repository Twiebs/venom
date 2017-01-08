layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_texcoord;
layout(location = 4) in ivec4 in_joint_indices;
layout(location = 5) in vec4 in_joint_weights;

layout(location = 00) uniform mat4 u_model_matrix;
layout(location = 01) uniform mat4 u_view_matrix;
layout(location = 02) uniform mat4 u_projection_matrix;
layout(location = 05) uniform mat4 uSkinningMatrices[64];
layout(location = 69) uniform int u_is_mesh_static;

out VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texcoord;
} vsOut;

void main() {
  vec4 vertex_position = vec4(in_position, 1.0);
  vec4 vertex_normal = vec4(in_normal, 0.0);
  
  if (u_is_mesh_static == 0) {
  	mat4 jointTransform = mat4(0.0);
  	jointTransform += uSkinningMatrices[in_joint_indices[0]] * in_joint_weights[0];
  	jointTransform += uSkinningMatrices[in_joint_indices[1]] * in_joint_weights[1];
  	jointTransform += uSkinningMatrices[in_joint_indices[2]] * in_joint_weights[2];
  	jointTransform += uSkinningMatrices[in_joint_indices[3]] * in_joint_weights[3];
  	vertex_position = jointTransform * vec4(in_position, 1.0);
  	vertex_normal = jointTransform * vec4(in_normal, 0.0);
  }
  
  mat4 model_view_matrix = u_view_matrix * u_model_matrix;
  mat4 normalMatrix = transpose(inverse(u_model_matrix));

  vertex_position.w = 1.0;
  vec4 worldspacePosition = u_model_matrix * vertex_position;
  worldspacePosition.w = 1.0;

  vsOut.position = worldspacePosition.xyz;
  vsOut.normal = vec3(normalMatrix * vertex_normal);
  vsOut.tangent = in_tangent;
  vsOut.texcoord = in_texcoord;
  gl_Position = u_projection_matrix * u_view_matrix * worldspacePosition; 
}