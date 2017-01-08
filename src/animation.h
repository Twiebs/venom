
enum AnimationType {
  AnimationType_Idle,
  AnimationType_Walk,
  AnimationType_Run,
  AnimationType_Count,
  AnimationType_Invalid,
};

enum AnimationBlendMode {
  AnimationBlendMode_None,
  AnimationBlendMode_SmoothCrossFade,
  AnimationBlendMode_FrozenCrossFade,
};

struct Joint_Translation_Info {
  F32 time;
  V3 translation;
};

struct Joint_Rotation_Info {
  F32 time;
  Quaternion rotation;
};

struct Joint_Scale_Info {
  F32 time;
  F32 scale;
};

struct Animation_Keyframe {
  F32 time;
  V3 translation;
  Quaternion rotation;
  F32 scale;
};

//TODO(Torin) Consider packing scale into translations for alignemtn + packing
//TODO(Torin) Remove the test dynamic arrays
//TODO(Torin) More clear name for this
struct Joint_Animation {
  U32 joint_index;
  U32 translation_count;
  U32 rotation_count;
  U32 scaling_count;
  DynamicArray<Joint_Translation_Info> translations;
  DynamicArray<Joint_Rotation_Info> rotations;
  DynamicArray<Joint_Scale_Info> scalings;
};

struct Animation_Joint {
  //TODO(Torin) Change joint indices to U8!
  S32 parent_index;
  S32 sibling_index;
  S32 child_index;

  M4 localTransform;
  M4 globalTransform;
  M4 inverseBindPose;
  char name[64];
};

struct Animation_Clip {
  AnimationType type;
  char name[64];
  F32 durationInTicks;
  F32 ticksPerSecond;
  U32 joint_count;
  DynamicArray<Joint_Animation> joint_animations;
};

struct AnimationClipState {
  U32 animationClipID;
  F32 localTimeSeconds;
  F32 playbackSpeedScalar;
  F32 blendWeightValue;
};

struct AnimationState {
  B8 isInitalized;
  Asset_ID model_id;
  F32 blendElapsedTime;
  F32 blendDurationTime;
  AnimationBlendMode blendMode;
  AnimationClipState animationStates[4];
  U32 animationStateCount;
  U32 localPoseOffset;
  U32 globalPoseOffset;
};

M4 CalculateSkinningMatrix(Animation_Joint *joint, M4 globalPose);
void CalculateGlobalPosesForSkeleton(Animation_Joint *jointList, size_t count, M4 *localPoses, M4 *globalPoses);
void CalculateLocalPosesForSkeleton(Animation_Joint *jointList, size_t count, AnimationState *animState, M4 *localPoses);

