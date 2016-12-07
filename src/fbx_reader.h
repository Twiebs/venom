#ifndef FBX_READER_INCLUDE
#define FBX_READER_INCLUDE

#include <stdint.h>

typedef struct {
  uint8_t null;
} FBX_Data;

int load_fbx_from_memory(void *buffer, FBX_Data *fbx);
int load_fbx_from_file(const char *filename, FBX_Data *fbx);

//===========================================================================

#include <stdio.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif//_MSC_VER

typedef struct {
  uint8_t magic_string[21];
  uint8_t unknown[2];
  uint32_t version_number;
} FBX_Header;

typedef struct {
  uint32_t end_offset;
  uint32_t property_count;
  uint32_t property_list_size;
  uint8_t name_size;
  char name[0];
} FBX_Node_Record;

typedef struct {
  uint8_t type_code;
} FBX_Property_Record;

typedef struct {
  uint8_t type_code;
  uint8_t data[0];
} FBX_Scalar_Property;

typedef struct {
  uint8_t type_code;
  uint32_t array_count;
  uint32_t encoding;
  uint32_t compressed_size;
} FBX_Array_Property;

#ifdef _MSC_VER
#pragma pack(pop)
#endif//_MSC_VER

static const uint8_t FBX_TYPE_CODE_S16 = 'Y';
static const uint8_t FBX_TYPE_CODE_BOOL = 'C';
static const uint8_t FBX_TYPE_CODE_S32 = 'I';
static const uint8_t FBX_TYPE_CODE_F32 = 'F';
static const uint8_t FBX_TPYE_CODE_F64 = 'D';
static const uint8_t FBX_TYPE_CODE_S64 = 'L';

static const uint8_t FBX_TYPE_CODE_F32_ARRAY = 'f';
static const uint8_t FBX_TYPE_CODE_F64_ARRAY = 'd';
static const uint8_t FBX_TYPE_CODE_S64_ARRAY = 'l';
static const uint8_t FBX_TYPE_CODE_S32_ARRAY = 'i';
static const uint8_t FBX_TYPE_CODE_BOOL_ARRAY = 'b';

static int indent_level = 0;
static int node_index = 0;

static inline int characters_are_equal(const char *a, const char *b, size_t length) {
  for (size_t i = 0; i < length; i++)
    if (a[i] != b[i]) return 0;
  return 1;
}

static FBX_Node_Record *process_node(FBX_Node_Record *node, size_t *current_file_offset) {
  for (size_t i = 0; i < indent_level; i++) printf("  ");
  printf("%.*s: %d\n", node->name_size, node->name, node_index);

  node_index++;

  FBX_Property_Record *property_array = (FBX_Property_Record *)(((uintptr_t)(node + 1)) + node->name_size);


  FBX_Node_Record *first_nested_node = (FBX_Node_Record *)((uintptr_t)property_array + node->property_list_size);
  FBX_Node_Record *current_nested_node = first_nested_node;
  *current_file_offset += (uintptr_t)current_nested_node - (uintptr_t)node;
  indent_level++;
  while (*current_file_offset < (node->end_offset - 13)) {
    current_nested_node = process_node(current_nested_node, current_file_offset);
  }
  indent_level--;

  FBX_Node_Record *next_node = current_nested_node;
  if (current_nested_node->end_offset == 0) {
    //assert((node->end_offset - *current_file_offset) == 13);
    next_node = (FBX_Node_Record *)((uintptr_t)current_nested_node + 13);
    *current_file_offset += 13;
  }

  return next_node;
}

int load_fbx_from_memory(void *buffer, size_t buffer_size, FBX_Data *fbx) {
  static const char *MAGIC_STRING = "Kaydara FBX Binary  ";
  FBX_Header *header = (FBX_Header *)buffer;
  for (size_t i = 0; i < sizeof(header->magic_string); i++) {
    if (header->magic_string[i] != MAGIC_STRING[i]) {
      printf("fbx magic string is invalid!\n");
      return 0;
    }
  }

  FBX_Node_Record *current_node = (FBX_Node_Record *)(header + 1);
  size_t current_file_offset = sizeof(FBX_Header);
  while (current_node->end_offset != 0) {
    process_node(current_node, &current_file_offset);
    current_node = (FBX_Node_Record *)((uintptr_t)buffer + current_node->end_offset);
  }
}

int load_fbx_from_file(const char *filename, FBX_Data *fbx) {
  FILE *file = fopen(filename, "rb");
  if (file == 0) return 0;
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  uint8_t *buffer = malloc(file_size);
  fread(buffer, file_size, 1, file);
  fclose(file);
  int result = load_fbx_from_memory(buffer, file_size, fbx);
  free(buffer);
  return result;
}

#endif//FBX_READER_INCLUDE