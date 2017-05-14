
inline V4 ProjectViewportCoordsToWorldSpaceRay(Camera *camera, S32 viewportX, S32 viewportY, S32 viewportWidth, S32 viewportHeight) {
  F32 x = (((F32)viewportX / (F32)viewportWidth) * 2.0f) - 1.0f;
  F32 y = (((F32)viewportY / (F32)viewportHeight) * 2.0f) - 1.0f;
  V4 screenspaceRay = { x, y, -1.0f, 1.0f };
  V4 viewspaceRay = Inverse(camera->projectionMatrix) * screenspaceRay;
  viewspaceRay = V4{ viewspaceRay.x, -viewspaceRay.y, -1.0f, 0.0 };
  V4 worldRay = Normalize(Inverse(camera->viewMatrix) * viewspaceRay);
  return worldRay;
}

inline void UpdateCamera(Camera *camera, V3 up) {
  camera->front.x = cosf(camera->yaw) * cosf(camera->pitch);
  camera->front.y = sinf(camera->pitch);
  camera->front.z = sin(camera->yaw) * cos(camera->pitch);
  camera->front = Normalize(camera->front);
  camera->viewMatrix = LookAt(camera->position, camera->front + camera->position, up);
}

inline void InitializeCamera(Camera *camera, F32 fov, F32 nearClip, F32 farClip, F32 viewportWidth, F32 viewportHeight) {
  camera->position = V3(0.0f, 0.0f, 0.0f);
  camera->yaw = -90 * DEG2RAD;
  camera->pitch = 0.0f;
  camera->fieldOfView = fov;
  camera->nearClip = nearClip;
  camera->farClip = farClip;
  camera->projectionMatrix = Perspective(camera->fieldOfView, viewportWidth, viewportHeight, camera->nearClip, camera->farClip);
}