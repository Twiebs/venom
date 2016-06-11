enum EditorCommand {
  EditorCommand_None,
  EditorCommand_Select,
  EditorCommand_Grab,
  EditorCommand_Project,
};

enum EditorSelectMode {
  EditorSelectMode_Default,
  EditorSelectMode_Append,
  EditorSelectMode_Remove,
};

const char *EditorCommandNames[] = {
  "EditorCommand_None",
  "EditorCommand_Select",
  "EditorCommand_Grab",
  "EditorCommand_Project",
};

struct EditorData {
  EditorCommand lastCommand;
  EditorCommand activeCommand;
  EditorSelectMode selectMode;

//TODO(Torin) Implement this camera functionality
  Camera *selectedCamera;
  Camera defaultCamera;
  DynamicArray<Camera> customCameras;

  DynamicArray<U32> selectedEntities;
  DynamicArray<V3> originalEntityPositions;

  AABB groupAABB;

  V3 originalGroupPosition;
  V3 currentGroupPosition;
  V3 currentGroupRotation;
  
};
