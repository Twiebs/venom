
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