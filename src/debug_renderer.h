
enum DebugDrawCommandType {
  DebugDrawCommand_BOX,
  DebugDrawCommand_SPHERE,
  DebugDrawCommand_LINE,
  DebugDrawCommand_AXIS,
  DebugDrawCommand_PLANE,
  DebugDrawCommand_COUNT
};

struct DebugDrawCommand {
  DebugDrawCommandType type;
  bool isSolid;
  V4 color;
  F32 duration;

  union {
    struct {
      V3 min, max;
    };
    struct {
      V3 center;
      F32 radius;
    };
    V3 position;
    V3 lineSegmentPositions[2];
  };
};

struct DebugRenderer {
  U32 vao, vbo, ebo;
  U32 cubeIndexOffset;
  U32 cubeIndexCount;
  U32 sphereIndexOffset;
  U32 sphereIndexCount;
  U32 axisIndexOffset;
  U32 axisIndexCount;
  U32 monkeyIndexCount;
  U32 monkeyIndexOffset;
  DynamicArray<DebugDrawCommand> drawCommands;
};

void initalize_debug_renderer();
void render_debug_draw_commands(Camera *camera, AssetManifest *assetManifest, float deltaTime);

void draw_debug_box(V3 min, V3 max, V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f);
void draw_debug_sphere(V3 center, F32 radius, V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f);
void draw_debug_line(V3 start, V3 end, V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f);
void draw_debug_axes(V3 position);
void draw_debug_plane(V3 a, V3 b, V3 c, V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f);