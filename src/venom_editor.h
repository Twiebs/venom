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

const char *EditorCommandNames[] = {
  "EditorCommand_None",
  "EditorCommand_Confirm",
  "EditorCommand_Cancel",

  "EditorCommand_Select",
  "EditorCommand_Grab",
  "EditorCommand_Project",
  "EditorCommand_MeshEdit",
};

struct EditorData {
  EditorCommand lastCommand;
  EditorCommand activeCommand;
  EditorSelectMode selectMode;
  EditorTransformConstraint transformConstraints;

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
