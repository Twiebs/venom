
struct ActorMovementParams {
  F32 stopScalar;
  F32 runAcceleration;
  F32 runMaxSpeed;
  F32 sprintAcceleration;
  F32 sprintMaxSpeed;
};

struct PlayerInputSettings {
  F32 mouseRotationSpeed;
};

struct PlayerInput {
  U8 foward;   //0-255
  U8 backward; //0-255
  U8 left;     //0-255
  U8 right;    //0-255
  S16 cursorDeltaX;
  S16 cursorDeltaY;
  B8 lookModeActive;
  B8 isSprintActive;
};