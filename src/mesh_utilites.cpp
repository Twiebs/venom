
void normalize_vertex_joint_weights(AnimatedVertex *vertices, size_t count) {
  for (size_t i = 0; i < count; i++) {
    AnimatedVertex *vertex = &vertices[i];
    float weight_sum = 0.0f;
    for (size_t j = 0; j < 4; j++) {
      weight_sum += vertex->weight[j];
    }
    for (size_t j = 0; j < 4; j++) {
      vertex->weight[j] /= weight_sum;
    }
  }
}

