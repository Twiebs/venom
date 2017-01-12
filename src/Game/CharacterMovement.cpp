
struct CharacterMovementParameters {
  F32 acceleration;
  F32 maxVelocity;
  F32 stopScalar;
};

struct MovementInput {
  U8 foward;   //0-255
  U8 backward; //0-255
  U8 left;     //0-255
  U8 right;    //0-255
};

MovementInput KeyboardToMovementInput(InputState *input) {
  MovementInput result = {};
  result.foward = input->isKeyDown[KEYCODE_W] ? 255 : 0;
  result.backward = input->isKeyDown[KEYCODE_S] ? 255 : 0;
  result.left = input->isKeyDown[KEYCODE_A] ? 255 : 0;
  result.right = input->isKeyDown[KEYCODE_D] ? 255 : 0;
  return result;
}

void MoveEntityWithThirdPersonCharacterMovement(Entity *e, MovementInput *input, CharacterMovementParameters *params, F32 deltaTime) {
  V3 deltaVelocity = V3(0.0f);
  if (input->foward) deltaVelocity -= V3(0, 0, 1.0f);
  if (input->backward) deltaVelocity += V3(0, 0, 1.0f);
  if (input->left) deltaVelocity -= V3(1.0f, 0, 0);
  if (input->right) deltaVelocity += V3(1.0f, 0, 0);

#if 1
  AnimationState *animState = &e->animation_state;
  static F32 elapsedTime = 0.0f;
  elapsedTime += deltaTime;
  if (elapsedTime > 3.0f) {
    SetAnimation(animState, AnimationType_Walk);
    elapsedTime = 0.0f;
  }
#endif

  F32 dVMagnitude2 = abs(MagnitudeSquared(deltaVelocity));
  F32 absVelocityMag2 = abs(MagnitudeSquared(e->velocity));
  if (dVMagnitude2 > 0.01f) {
    deltaVelocity = Normalize(deltaVelocity) * params->acceleration;
    deltaVelocity *= deltaTime;
    e->velocity += deltaVelocity;

  } else {
    e->velocity *= params->stopScalar;

  }

  F32 magnitudeSquared = abs(MagnitudeSquared(e->velocity));
  if (magnitudeSquared > 0.1f) {
    F32 magnitude = sqrtf(magnitudeSquared);
    if (magnitude > params->maxVelocity) {
      V3 clampedVelocity = Normalize(e->velocity);
      clampedVelocity *= params->maxVelocity;
      e->velocity = clampedVelocity;
    }

    V3 velocityDirection = Normalize(V3(-e->velocity.x, 0.0f, e->velocity.z));
    M4 rotationMatrix = DirectionToRotationMatrix(velocityDirection);
    Quaternion targetRotation = MatrixToQuaternion(rotationMatrix);
    e->rotation = Lerp(e->rotation, targetRotation, 0.1f);
  }
}