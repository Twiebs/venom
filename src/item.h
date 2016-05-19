
enum ItemType {
  ItemType_COMPONENT,
  ItemType_WEAPON,
};

struct Item {
  ItemType type;
};

struct ItemContainer {
  static const U32 MAX_ITEMS = 16;
  Item items[MAX_ITEMS];
};



