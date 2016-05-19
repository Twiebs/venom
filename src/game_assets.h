#define _(name, flags) MaterialID_##name,
enum MaterialID { 
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE
#endif//VENOM_MATERIAL_LIST_FILE
  MaterialID_COUNT 
};
#undef _

#define _(name, flags) #name,
const char *MATERIAL_NAMES[] { 
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE
#endif//VENOM_MATERIAL_LIST_FILE
}; 
#undef _

