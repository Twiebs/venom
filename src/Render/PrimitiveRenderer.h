
struct TextDrawCommand {
  V3 position;
  F32 duration;
  V4 color;
  const char *text;
};

struct PrimitiveVertex {
  V3 position;
};

class PrimitiveRenderer {
  DynamicArray<DebugDrawCommand> drawCommands;
  DynamicArray<TextDrawCommand> textCommands;

  V3 lightDirection;
  DebugDrawCommand *AddDrawCommand(DebugDrawCommandType type, V4 color, B8 isSolid, F32 duration);

public:
  B8 isLightingEnabled;

  void Render(const Camera *camera, const F32 deltaTime);

  void DirectionalLight(V3 direction);

  void Box(const AABB& a, V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f);
  void Box(V3 min, V3 max, V4 color = COLOR_WHITE, B8 isSolid = false, F32 duration = 0.0f);
  void Sphere(V3 center, F32 radius, V4 color = COLOR_WHITE, B8 isSolid = false, F32 duration = 0.0f);
  void Line(V3 start, V3 end, V4 color = COLOR_WHITE, B8 isSolid = false, F32 duration = 0.0f);
  void Triangle(V3 a, V3 b, V3 c, V4 color = COLOR_WHITE, B8 isSolid = false, F32 duration = 0.0f);

  void Axis(V3 position, F32 duration = 0.0f);
  void Text(const char *text, V3 position, V4 color = COLOR_WHITE, F32 duration = 0.0f);
};