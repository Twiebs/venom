
#include "CharacterMovement.h"

inline PlayerInput KeyboardAndMouseToPlayerInput(InputState *input) {
  PlayerInput result = {};
  result.foward = input->isKeyDown[KEYCODE_W] ? 255 : 0;
  result.backward = input->isKeyDown[KEYCODE_S] ? 255 : 0;
  result.left = input->isKeyDown[KEYCODE_A] ? 255 : 0;
  result.right = input->isKeyDown[KEYCODE_D] ? 255 : 0;
  result.cursorDeltaX = input->cursorDeltaX;
  result.cursorDeltaY = input->cursorDeltaY;
  result.lookModeActive = input->isButtonDown[MOUSE_RIGHT];
  result.isSprintActive = input->isKeyDown[KEYCODE_SHIFT];
  return result;
}

void ThirdPersonPlayerControl(Entity *entity, Camera *camera, PlayerInput *input, F32 deltaTime) {
  struct PlayerInputSettings settings = {};
  struct ActorMovementParams params = {};
  settings.mouseRotationSpeed = 0.007f;
  params.runAcceleration = 30.0f;
  params.runMaxSpeed = 8.0f;
  params.sprintMaxSpeed = 16.0f;
  params.sprintAcceleration = 30.0f;
  params.stopScalar = 0.9f;
  
  auto animState = &entity->animation_state;
  
  V3 deltaVelocity = V3(0.0f);
  if (input->foward) deltaVelocity -= V3(0, 0, 1.0f);
  if (input->backward) deltaVelocity += V3(0, 0, 1.0f);
  if (input->left) deltaVelocity -= V3(1.0f, 0, 0);
  if (input->right) deltaVelocity += V3(1.0f, 0, 0);

  F32 dVMagnitude2 = abs(MagnitudeSquared(deltaVelocity));
  if (dVMagnitude2 > 0.01f) {
    M4 fowardMovementMatrix = Rotate(0.0f, entity->movementFacingAngle, 0.0f);
    deltaVelocity = Normalize(V3(fowardMovementMatrix * V4(deltaVelocity, 0.0f)));
    if (input->isSprintActive) {
      deltaVelocity *= params.sprintAcceleration;
    } else {
      deltaVelocity *= params.runAcceleration;
    }

    deltaVelocity *= deltaTime;
    entity->velocity += deltaVelocity;
    if (animState->animationStates[0].animationClipID == 0) {
      CrossFade(animState, 0.4f, 0, 1);
    }
  } else {
    entity->velocity.x *= params.stopScalar;
    entity->velocity.z *= params.stopScalar;
    if (animState->animationStates[0].animationClipID == 1) {
      CrossFade(animState, 0.4f, 1, 0);
    }
  }

  F32 magnitudeSquared = abs(MagnitudeSquared(V3(entity->velocity.x, 0.0f, entity->velocity.z)));
  if (magnitudeSquared > 0.1f) {
    F32 magnitude = sqrtf(magnitudeSquared);
    F32 maxSpeed = input->isSprintActive ? params.sprintMaxSpeed : params.runMaxSpeed;
    if (magnitude > maxSpeed) {
      entity->velocity = Normalize(entity->velocity) * maxSpeed;
    }

    V3 velocityDirection = Normalize(V3(-entity->velocity.x, 0.0f, entity->velocity.z));
    M4 rotationMatrix = DirectionToRotationMatrix(velocityDirection);
    Quaternion targetRotation = MatrixToQuaternion(rotationMatrix);
    entity->rotation = Lerp(entity->rotation, targetRotation, 0.1f);
  }

  if (input->lookModeActive) {
    entity->movementFacingAngle += input->cursorDeltaX * settings.mouseRotationSpeed;
    F32 targetPitch = camera->pitch + input->cursorDeltaY * 0.07f;
    camera->pitch = Lerp(camera->pitch, targetPitch, 0.1f);

#if 0
    Quaternion cameraRotation = QuaternionFromEulerAngles(camera->pitch, camera->yaw, 0.0f);
    Quaternion targetCameraRotation = QuaternionFromEulerAngles(-22.0f*DEG2RAD, -eulerPlayerRotation.y, 0.0f);
    Quaternion currentRotation = Lerp(cameraRotation, targetCameraRotation, 0.2f);
    V3 rotationEuler = QuaternionToEuler(currentRotation);
    camera->pitch = rotationEuler.x;
    camera->yaw = rotationEuler.y;
#endif
  } else {
    camera->pitch = Lerp(camera->pitch, -20.0f*DEG2RAD, 0.005f);
  }

  //Set the pitch and yaw of the camera by slerping from the current to target
  camera->yaw = -90.0*DEG2RAD - entity->movementFacingAngle;
  Clamp(camera->pitch, -89.0*DEG2RAD, 89.0*DEG2RAD);

  //Calculate the cameras position based on the rotation
  V3 cameraFront = {};
  cameraFront.x = cos(camera->yaw) * cos(camera->pitch);
  cameraFront.y = sin(camera->pitch);
  cameraFront.z = sin(camera->yaw) * cos(camera->pitch);
  cameraFront = Normalize(cameraFront);
  camera->position = entity->position + V3(0.0f, 1.0f, 0.0f) - (cameraFront * 24.0f);
}