
enum EditorCommand {
  EditorCommand_None,
  EditorCommand_Confirm,
  EditorCommand_Cancel,

  EditorCommand_Select,
  EditorCommand_Grab,
  EditorCommand_Project,
  EditorCommand_MeshEdit,
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
  EditorViewMode_Debug,
  EditorViewMode_Player,
};

struct EditorData {
  EditorCommand lastCommand;
  EditorCommand activeCommand;
  EditorSelectMode selectMode;
  EditorViewMode viewMode;
  EditorTransformConstraint transformConstraints;

  Camera *activeCamera;
  Camera editorCamera;

  //EditorViewMode_Assets
  int selectedAssetType;
  int lastSelectedIndex;
  int selectedIndex;

  //EditorViewMode_Entities
  DynamicArray<U32> selectedEntities;
  DynamicArray<V3> originalEntityPositions;
  V3 originalGroupPosition;
  V3 currentGroupPosition;
  V3 currentGroupRotation;
  AABB groupAABB;

  B8 isEditorVisible;
  B8 isSearchWindowOpen;
};

void InitalizeEditor(EditorData *editor);