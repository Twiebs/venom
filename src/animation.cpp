//XXX(Torin) Make sure that the animation joinst keyframes are sorted by time value!
//TODO(Torin) Make sure that each animation clip has at least 2 keyframes

M4 CalculateLocalJointPose(S32 joint_index, Animation_Joint *joint, Animation_State *state) {
  ModelAsset *model = GetModelAsset(state->model_id);
  if (model == nullptr) return M4Identity();
  if (model->data.animation_clip_count == 0 || state->current_clip >= model->data.animation_clip_count) return M4Identity();
  Animation_Clip *clip = &model->data.animation_clips[state->current_clip];

  F32 inverse_fps = 1.0f;
  F32 current_animation_time = fmod(state->animation_time, clip->duration);
  M4 joint_translation = M4Identity();
  M4 joint_rotation = M4Identity();
  M4 joint_scaling = M4Identity();

  S32 joint_animation_index = -1;
  for (size_t i = 0; i < clip->joint_count; i++) {
    if (clip->joint_animations[i].joint_index == joint_index) {
      joint_animation_index = i;
    }
  }

  if (joint_animation_index != -1) {
    Joint_Animation *joint_animation = &clip->joint_animations[joint_animation_index];
    size_t start_index = 0;
    size_t end_index = 0;
    for (size_t i = 0; i < joint_animation->translation_count; i++) {
      if (current_animation_time > (joint_animation->translations[i].time*inverse_fps) &&
        current_animation_time < (joint_animation->translations[(i + 1) % joint_animation->translation_count].time*inverse_fps)) {
        start_index = i;
        end_index = (i + 1) % joint_animation->translation_count;
        break;
      }
    }

    if (start_index != end_index) {
      Joint_Translation_Info *start_translation = &joint_animation->translations[start_index];
      Joint_Translation_Info *end_translation = &joint_animation->translations[end_index];
      F32 transition_length = abs((end_translation->time*inverse_fps) - (start_translation->time*inverse_fps));
      F32 normalized_animation_time = current_animation_time - (start_translation->time*inverse_fps);
      F32 time_interp = normalized_animation_time / transition_length;
      V3 current_translation = lerp(start_translation->translation, end_translation->translation, time_interp);
      joint_translation = Translate(current_translation);
    } else {
      LogWarning("start_index == end_index");
    }


    { //Rotation

      assert(current_animation_time < joint_animation->rotations[joint_animation->rotation_count-1].time);
      size_t start_index = 0;
      for (size_t i = 0; i < joint_animation->rotation_count - 1; i++) {
        Joint_Rotation_Info *rotation_info = &joint_animation->rotations[i];
        if (current_animation_time >= rotation_info->time) {
          start_index = i;
        }
      }

      size_t end_index = start_index + 1;

      Joint_Rotation_Info *start_rotation = &joint_animation->rotations[start_index];
      Joint_Rotation_Info *end_rotation = &joint_animation->rotations[end_index];
      F32 time_between_keyframes = abs(end_rotation->time - start_rotation->time);
      F32 normalized_local_time = current_animation_time - start_rotation->time;
      F32 interpolation_constant = normalized_local_time / time_between_keyframes;
      Quaternion interpolated_rotation = Lerp(start_rotation->rotation, end_rotation->rotation, interpolation_constant);
      joint_rotation = QuaternionToMatrix(interpolated_rotation);
    }
  }

  M4 result = joint_rotation;
  return result;
}


M4 CalculateGlobalJointPose(S32 joint_index, Animation_Joint *joint_list, M4 *local_poses) {
  M4 result = local_poses[joint_index];
  Animation_Joint *joint = &joint_list[joint_index];
  S32 parent_index = joint->parent_index;
  while (parent_index != -1) {
    result = local_poses[parent_index] * result;
    Animation_Joint *parent = &joint_list[parent_index];
    parent_index = parent->parent_index;
  }

  //result = joint_list[0].parent_realtive_matrix * result;
  return result;
}