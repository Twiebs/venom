enum EditorCommand {
  EditorCommand_None,
  EditorCommand_Select,
  EditorCommand_Grab,
  EditorCommand_Project,
};

const char *EditorCommandNames[] = {
  "EditorCommand_None",
  "EditorCommand_Select",
  "EditorCommand_Grab",
  "EditorCommand_Project",
};

struct EditorData {
//TODO(Torin: Jun 09, 2016) Camera does not belong here
  Camera editorCamera; 
  //U32 selectedEntityIndex;

  DynamicArray<U32> selectedEntities;
  V3 entityGroupPosition;
  V3 entityGroupRotation;


  EditorCommand activeCommand;
};
