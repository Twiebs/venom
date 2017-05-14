//XXX(Torin) Make sure that the animation joinst keyframes are sorted by time value!
//TODO(Torin) Make sure that each animation clip has at least 2 keyframes

M4 CalculateLocalJointPoseForClip(S32 joint_index, Animation_Joint *joint, ModelAsset *model, AnimationClipState *state) {
  Animation_Clip *clip = &model->animationClips[state->animationClipID];
  F32 inverseTicksPerSecond = 1.0f / clip->ticksPerSecond;
  F32 durationInSeconds = inverseTicksPerSecond * clip->durationInTicks;
  F32 current_animation_time = fmod(state->localTimeSeconds, durationInSeconds);

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

    { //Translation
      size_t start_index = 0;
      size_t end_index = 0;
      for (size_t i = 0; i < joint_animation->translation_count; i++) {
        start_index = i;
        end_index = (start_index + 1) % joint_animation->translation_count;
        assert(start_index != end_index); //Impossible!
        F32 start_keyframe_time = joint_animation->translations[start_index].time*inverseTicksPerSecond;
        F32 end_keyframe_time = joint_animation->translations[end_index].time*inverseTicksPerSecond;
        if ((current_animation_time > start_keyframe_time) && (current_animation_time < end_keyframe_time)) {
          break;
        }
      }

      assert(start_index != end_index);

      Joint_Translation_Info *start_translation = &joint_animation->translations[start_index];
      Joint_Translation_Info *end_translation = &joint_animation->translations[end_index];
      F32 transition_length = abs((end_translation->time*inverseTicksPerSecond) - (start_translation->time*inverseTicksPerSecond));
      F32 normalized_animation_time = current_animation_time - (start_translation->time*inverseTicksPerSecond);
      F32 time_interp = normalized_animation_time / transition_length;
      V3 current_translation = Lerp(start_translation->translation, end_translation->translation, time_interp);
      joint_translation = Translate(current_translation);
    }

    { //Rotation

      assert(current_animation_time < joint_animation->rotations[joint_animation->rotation_count - 1].time);
      size_t start_index = 0;
      for (size_t i = 0; i < joint_animation->rotation_count - 1; i++) {
        Joint_Rotation_Info *rotation_info = &joint_animation->rotations[i];
        if (current_animation_time >= (rotation_info->time*inverseTicksPerSecond)) {
          start_index = i;
        }
      }

      size_t end_index = start_index + 1;

      Joint_Rotation_Info *start_rotation = &joint_animation->rotations[start_index];
      Joint_Rotation_Info *end_rotation = &joint_animation->rotations[end_index];
      F32 time_between_keyframes = abs(end_rotation->time*inverseTicksPerSecond - start_rotation->time*inverseTicksPerSecond);
      F32 normalized_local_time = current_animation_time - start_rotation->time*inverseTicksPerSecond;
      F32 interpolation_constant = normalized_local_time / time_between_keyframes;
      Quaternion interpolated_rotation = Lerp(start_rotation->rotation, end_rotation->rotation, interpolation_constant);
      joint_rotation = QuaternionToMatrix4x4(interpolated_rotation);
    }
  } else {
    return joint->localTransform;
  }

  M4 result = joint_translation * joint_rotation;
  return result;
}

#define DebugCodeBlock if(1) 

M4 CalculateGlobalJointPose(S32 joint_index, Animation_Joint *joint_list, M4 *local_poses) {
  Animation_Joint *joint = &joint_list[joint_index];
  M4 transform = local_poses[joint_index];
  S32 parent_index = joint->parent_index;
  while (parent_index != -1) {
    transform = local_poses[parent_index] * transform;
    Animation_Joint *parent = &joint_list[parent_index];
    parent_index = parent->parent_index;
  }

  return transform;
}

void CalculateGlobalPosesForSkeleton(Animation_Joint *jointList, size_t count, M4 *localPoses, M4 *globalPoses) {
  for (size_t i = 0; i < count; i++) {
    globalPoses[i] = CalculateGlobalJointPose((S32)i, jointList, localPoses);
  }
}

void CalculateLocalPosesForSkeleton(ModelAsset *model, AnimationState *animState, M4 *localPoses) {
  assert(model != nullptr);

}

M4 CalculateSkinningMatrix(Animation_Joint *joint, M4 globalPose) {
  return globalPose * joint->inverseBindPose;
}

static U32 FindAnimationClipIndex(Animation_Clip *clips, size_t count, AnimationType type) {
  for (size_t i = 0; i < count; i++)
    if (clips[i].type == type) return i;
  return INVALID_U32;
}

//TODO(Torin 2016-12-24) Remove duplication of modelID in animState and entity

void UpdateAnimationState(AnimationState *animState, ModelAsset *model, F32 deltaTime) {
  //NOTE(Torin 2017-01-12) This happens here because the model may have just been loaded into
  //memory by the async asset system but the entities animState existed before the model was fully loaded.
  if (animState->isInitalized == false) {
    animState->isInitalized = true;
    animState->blendDurationTime = 0.0f;
    animState->blendElapsedTime = 0.0f;
    animState->blendMode = AnimationBlendMode_None;
    animState->animationStateCount = 1;
    AnimationClipState *clipState = &animState->animationStates[0];
    clipState->blendWeightValue = 1.0f;
    clipState->animationClipID = FindAnimationClipIndex(model->animationClips, model->animationClipCount, AnimationType_Idle);
    clipState->playbackSpeedScalar = 1.0f;
  }

  for (size_t i = 0; i < animState->animationStateCount; i++) {
    animState->animationStates[i].localTimeSeconds += deltaTime;
  }

  if (animState->blendMode != AnimationBlendMode_None) {
    animState->blendElapsedTime += deltaTime;
    if (animState->blendElapsedTime > animState->blendDurationTime) {
      animState->blendElapsedTime = 0.0f;
      animState->blendDurationTime = 0.0f;
      animState->blendMode = AnimationBlendMode_None;
      AnimationClipState *clipState0 = &animState->animationStates[0];
      AnimationClipState *clipState1 = &animState->animationStates[1];
      clipState0->blendWeightValue = 1.0f;
      clipState0->animationClipID = clipState1->animationClipID;
      animState->animationStateCount = 1;
    } else {
      assert(animState->animationStateCount == 2);
      AnimationClipState *clipState0 = &animState->animationStates[0];
      AnimationClipState *clipState1 = &animState->animationStates[1];
      F32 elapsedOverDuration = animState->blendElapsedTime / animState->blendDurationTime;
      clipState0->blendWeightValue = 1.0f - elapsedOverDuration;
      clipState1->blendWeightValue = elapsedOverDuration;
    }
  }

  DebugCodeBlock {
    F32 totalBlendWeightValue = 0.0f;
    for (size_t i = 0; i < animState->animationStateCount; i++)
      totalBlendWeightValue += animState->animationStates[i].blendWeightValue;
    assert(Equals(totalBlendWeightValue, 1.0f));
  }

  assert(model != nullptr);
  assert(model->animationClipCount > 0);
  assert(animState->animationStateCount > 0);
  animState->localPoseOffset = Memory::FrameStackPush(model->jointCount * sizeof(M4));
  animState->globalPoseOffset = Memory::FrameStackPush(model->jointCount * sizeof(M4));
  M4 *localPoses = (M4 *)Memory::FrameStackPointer(animState->localPoseOffset);
  M4 *globalPoses = (M4 *)Memory::FrameStackPointer(animState->globalPoseOffset);

  //Calculate local poses for this animation state
  for (size_t i = 0; i < model->jointCount; i++) {
    Animation_Joint *joint = &model->joints[i];
    localPoses[i] = M4Zero();

    for (size_t j = 0; j < animState->animationStateCount; j++) {
      AnimationClipState *clipState = &animState->animationStates[j];
      M4 clipLocalPose = CalculateLocalJointPoseForClip(i, joint, model, clipState);
      localPoses[i] = localPoses[i] + (clipLocalPose * clipState->blendWeightValue);
    }
  }

  CalculateGlobalPosesForSkeleton(model->joints, model->jointCount, localPoses, globalPoses);
}

void SetAnimation(AnimationState *animState, AnimationType type) {
  auto model = GetModelAsset(animState->model_id);
  U32 clipID = FindAnimationClipIndex(model->animationClips, model->animationClipCount, type);
  if (clipID != INVALID_U32) {
    animState->blendMode = AnimationBlendMode_None;
    animState->blendDurationTime = 0.0f;
    auto clipState = &animState->animationStates[0];
    clipState->animationClipID = clipID;
    clipState->blendWeightValue = 1.0f;
    clipState->localTimeSeconds = 0.0f;
    clipState->playbackSpeedScalar = 1.0f;
  }
  animState->animationStateCount = 1;
}

void CrossFade(AnimationState *animState, F32 durationInSeconds, U32 sourceClip, U32 destClip) {
  assert(animState->animationStateCount != 0);
  animState->blendMode = AnimationBlendMode_SmoothCrossFade;
  animState->blendDurationTime = durationInSeconds;
  AnimationClipState *clipState0 = &animState->animationStates[0];
  AnimationClipState *clipState1 = &animState->animationStates[1];
  clipState0->animationClipID = sourceClip;
  clipState0->blendWeightValue = 1.0f;
  clipState1->animationClipID = destClip;
  clipState1->blendWeightValue = 0.0f;
  clipState1->localTimeSeconds = 0.0f;
  animState->animationStateCount = 2;
}