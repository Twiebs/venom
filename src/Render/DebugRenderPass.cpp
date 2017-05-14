
static void DrawBone(S32 jointIndex, Animation_Joint *jointList, M4 *globalTransforms, M4 modelMatrix) {
  static const V4 LINE_COLOR = COLOR_YELLOW;
  Animation_Joint *joint = &jointList[jointIndex];
  if (joint->child_index != -1) {
    V4 start = globalTransforms[jointIndex] * V4(0.0f, 0.0f, 0.0f, 1.0f);
    V4 end = globalTransforms[joint->child_index] * V4(0.0f, 0.0f, 0.0f, 1.0f);
    start = modelMatrix * start;
    end = modelMatrix * end;
    draw_debug_line(V3(start), V3(end), LINE_COLOR);
  } else {
    V4 start = globalTransforms[jointIndex] * V4(0.0f, 0.0f, 0.0f, 1.0f);
    V4 end = globalTransforms[jointIndex] * V4(0.0f, 1.0f, 0.0f, 1.0f);
    start = modelMatrix * start;
    end = modelMatrix * end;
    draw_debug_line(V3(start), V3(end), LINE_COLOR);
  }
}

static inline void DrawSkeleton(Animation_Joint *jointList, size_t jointCount, AnimationState *animState, M4 modelMatrix) {
  M4 *globalJointPoses = (M4 *)Memory::FrameStackPointer(animState->globalPoseOffset);
  for (size_t i = 0; i < jointCount; i++) {
    Animation_Joint *joint = &jointList[i];
    DrawBone((S32)i, jointList, globalJointPoses, modelMatrix);
  }
}

static inline void DebugRenderPass(RenderState *rs) {
  auto settings = GetDebugRenderSettings();
  if (settings->drawSkeletons) {
    for (size_t i = 0; i < rs->drawList.animated_model_draw_commands.count; i++) {
      Animated_Model_Draw_Command *cmd = &rs->drawList.animated_model_draw_commands[i];
      ModelAsset *model = cmd->model;
      assert(model != nullptr);
      DrawSkeleton(model->joints, model->jointCount, cmd->animation_state, cmd->model_matrix);
    }
  }
}