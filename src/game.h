inline void 
MoveCameraWithFPSControls(Camera *camera, InputState* input, float deltaTime) {
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
