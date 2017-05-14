
struct Camera {
  V3 position;
  V3 front;

  F32 pitch;
  F32 yaw;

  F32 fieldOfView;
  F32 nearClip;
  F32 farClip;
  M4 viewMatrix;
  M4 projectionMatrix;
};

inline V4 ProjectViewportCoordsToWorldSpaceRay(Camera *camera, S32 viewportX, S32 viewportY, S32 viewportWidth, S32 viewportHeight);
inline void UpdateCamera(Camera *camera, V3 up = V3(0.0f, 1.0f, 0.0f));
inline void InitializeCamera(Camera *camera, F32 fov, F32 nearClip, F32 farClip, F32 viewportWidth, F32 viewportHeight);