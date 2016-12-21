
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
  char name[64];
  F32 duration;
  U32 joint_count;
  DynamicArray<Joint_Animation> joint_animations;
};

struct Animation_State {
  Asset_ID model_id;
  U32 current_clip;
  F32 animation_time;
  F32 frames_per_second;
};

M4 CalculateLocalJointPose(S32 joint_index, Animation_Joint *joint, Animation_State *state);
M4 CalculateGlobalJointPose(S32 joint_index, Animation_Joint *joint_list, M4 *local_poses);
M4 CalculateSkinningMatrix(Animation_Joint *joint, M4 globalPose);