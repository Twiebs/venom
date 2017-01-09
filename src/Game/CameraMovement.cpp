
//TODO(Torin) Move out this Orientation structure
struct Orientation {
  Quaternion rotation;
  V3 position;
};

inline Orientation CalculateCameraOrientationForTrackTarget(V3 targetPosition) {
  Orientation result = {};
  result.position = targetPosition;
  result.position.z += 20.0f;
  result.position.y += 10.0f;

  V3 cameraToTarget = result.position - targetPosition;
  cameraToTarget = Normalize(cameraToTarget);
  F32 cosViewAngle = Dot(cameraToTarget, V3(0.0f, 1.0f, 0.0f));
  F32 viewAngle = (-PI32*0.25f) * cosViewAngle;
  result.rotation = QuaternionFromEulerAngles(viewAngle, 0.0f, 0.0f);
  return result;
}

inline void TrackPositionWithCamera(V3 position, Camera *camera) {
  Orientation target = CalculateCameraOrientationForTrackTarget(position);
  V3 eulerRotation = QuaternionToEuler(target.rotation);
  camera->yaw = -PI32*0.5f;
  camera->pitch = eulerRotation.x;

  camera->position = Lerp(camera->position, target.position, 0.2f);
}

inline void MoveCameraWithFPSControls(Camera *camera, InputState* input, float deltaTime) {
  V3 dp = { 0.0f, 0.0f, 0.0f };

  float speed = 6.0f;
  if (input->isKeyDown[KEYCODE_W]) dp += camera->front;
  if (input->isKeyDown[KEYCODE_S]) dp -= camera->front;
  if (input->isKeyDown[KEYCODE_A]) dp -= Normalize(
    Cross(camera->front, V3(0.0f, 1.0f, 0.0f)));
  if (input->isKeyDown[KEYCODE_D]) dp += Normalize(
    Cross(camera->front, V3(0.0f, 1.0f, 0.0f)));
  if (input->isKeyDown[KEYCODE_SPACE]) dp.y += 1.0f;
  if (input->isKeyDown[KEYCODE_CTRL]) dp.y -= 1.0f;
  if (input->isKeyDown[KEYCODE_SHIFT]) speed *= 3;
  if (input->isKeyDown[KEYCODE_ALT]) speed *= 3;

  //dp = Normalize(dp);
  dp *= deltaTime * speed;
  camera->position += dp;

  camera->yaw -= input->cursorDeltaX * 0.01f;
  camera->pitch += input->cursorDeltaY * 0.01f;
  camera->pitch = Clamp(camera->pitch, -(PI32*0.5f - (1.0f*DEG2RAD)),
    PI32*0.5f - (1.0f*DEG2RAD));
}
