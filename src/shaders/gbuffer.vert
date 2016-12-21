layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_texcoord;

layout (location = 4) in ivec4 in_joint_indices;
layout (location = 5) in vec4 in_joint_weights;

layout (location = 0) uniform mat4 u_model_matrix;
layout (location = 1) uniform mat4 u_view_matrix;
layout (location = 2) uniform mat4 u_projection_matrix;
layout (location = 5) uniform mat4 u_joint_matrices[16];
layout (location = 21) uniform int u_is_mesh_static;

out VertexShaderOut {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texcoord;
} vsOut;


void main() {
  //mat4 modelViewMatrix = uViewMatrix * uModelMatrix;
  //vec4 viewspacePosition = modelViewMatrix * vec4(inPosition, 1.0);
  //vec3 viewspaceNormal = mat3(modelViewMatrix) * inNormal;
  //vsOut.position = viewspacePosition.xyz;
  //vsOut.normal = viewspaceNormal.xyz;

  vec4 vertex_position = vec4(0.0, 0.0, 0.0, 1.0);
  vec4 vertex_normal = vec4(0.0, 0.0, 0.0, 0.0);
  if (u_is_mesh_static == 0) {
  	vertex_position += (u_joint_matrices[in_joint_indices[0]] * vec4(in_position, 1.0)) * in_joint_weights[0];
  	vertex_position += (u_joint_matrices[in_joint_indices[1]] * vec4(in_position, 1.0)) * in_joint_weights[1];
  	vertex_position += (u_joint_matrices[in_joint_indices[2]] * vec4(in_position, 1.0)) * in_joint_weights[2];
  	vertex_position += (u_joint_matrices[in_joint_indices[3]] * vec4(in_position, 1.0)) * in_joint_weights[3];
    vertex_normal = normalize(vertex_normal);
  } 

  mat4 model_view_matrix = u_view_matrix * u_model_matrix;
  mat4 normalMatrix = transpose(inverse(u_model_matrix));

  vertex_position.w = 1.0;
  vec4 worldspacePosition = u_model_matrix * vertex_position;
  worldspacePosition.w = 1.0;

  vsOut.position = worldspacePosition.xyz;
  vsOut.normal = in_normal;
  //vsOut.normal = vec3(transformed_normal);
  vsOut.tangent = in_tangent;
  vsOut.texcoord = in_texcoord;
  gl_Position = u_projection_matrix * u_view_matrix * worldspacePosition; 
}