

void UpdateMousePicker(Camera *camera) {
  auto data = GetVenomEngineData();
  InputState *input = &data->inputState;
  SystemInfo *sys = &data->systemInfo;

  auto engine = GetEngine();
  V4 direction = ProjectViewportCoordsToWorldSpaceRay(camera, input->cursorPosX,
    input->cursorPosY, sys->screenWidth, sys->screenHeight);
  V3 terrainHitPoint = {};
  if (TestTerrainRayIntersection(&engine->terrain, camera->position, direction, &terrainHitPoint)) {
    draw_debug_sphere(terrainHitPoint, 2.0f, COLOR_RED, true);
  }
}