
enum EditorCommand {
  EditorCommand_None,
  EditorCommand_Confirm,
  EditorCommand_Cancel,

  EditorCommand_Select,
  EditorCommand_Grab,
  EditorCommand_Project,
  EditorCommand_MeshEdit,
};

enum EditorSelectMode {
  EditorSelectMode_Default,
  EditorSelectMode_Append,
  EditorSelectMode_Remove,
};

enum EditorTransformConstraint {
  EditorTransformConstraint_XAxis = 1 << 0,
  EditorTransformConstraint_YAxis = 1 << 1,
  EditorTransformConstraint_ZAxis = 1 << 2,
};

enum EditorViewMode {
  EditorViewMode_Assets,
  EditorViewMode_Entities,
  EditorViewMode_Physics,
  EditorViewMode_Render,
  EditorViewMode_Game,
};

enum VisualizerMode {
  VisualizerMode_GBufferColor,
  VisualizerMode_GBufferNormal,
  VisualizerMode_GBufferPosition,
  VisualizerMode_VisualizerScene,
};

const char *EditorCommandNames[] = {
  "EditorCommand_None",
  "EditorCommand_Confirm",
  "EditorCommand_Cancel",

  "EditorCommand_Select",
  "EditorCommand_Grab",
  "EditorCommand_Project",
  "EditorCommand_MeshEdit",
};

//TODO(Torin 2017-02-04) Unionize Editor
//Also just rename the editor editor!

struct EditorData {
  EditorCommand lastCommand;
  EditorCommand activeCommand;
  EditorSelectMode selectMode;
  EditorViewMode viewMode;
  EditorTransformConstraint transformConstraints;

  U32 activeCameraIndex;
  Camera editorCamera;
  DynamicArray<Camera *> cameraList;

  //EditorViewMode_Assets
  S32 selectedAssetType;
  S32 lastSelectedIndex;
  S32 selectedIndex;

  //EditorViewMode_Entities
  DynamicArray<U32> selectedEntities;
  DynamicArray<V3> originalEntityPositions;
  V3 originalGroupPosition;
  V3 currentGroupPosition;
  V3 currentGroupRotation;
  AABB groupAABB;

  //EditorViewMode_Physics
  U64 gjkStepIndex;

  //Visualizer
  PrimitiveRenderer visualizerRenderer;
  VisualizerMode visualizerMode;
  U32 visualizerWidth;
  U32 visualizerHeight;
  GLuint visualizerFramebuffer;
  GLuint visualizerColorBuffer;
  GLuint visualizerDepthBuffer;
  Camera visualizerCamera;

  //State bools
  B8 isVisualizerActive;
  B8 isVisualizerVisible;
  B8 isEditorVisible;
  B8 isSearchWindowOpen;
};


void InitalizeEditor(EditorData *editor);
static inline void update_editor_selection(U32 index, EditorData *editor, EntityContainer *ec, AssetManifest *am);