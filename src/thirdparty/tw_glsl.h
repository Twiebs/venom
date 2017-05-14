
//NOTE(Torin) 
//This is a quick and dirty way to get hotloadable GLSL shader programs
//into a project as quickly and seamlessly as possible.
//This is not intended to be a shipable solution.  

#include <stdint.h>
#include <vector>
#include <string>

namespace GLSL 
{

enum ShaderType {
  ShaderType_Vertex,
  ShaderType_Fragment,
  ShaderType_Geometry,
  ShaderType_Tessellation,
  ShaderType_Compute
};

struct ShaderFile {
  ShaderType type;
  uint64_t lastModifedTime;
  std::string path;
};

struct ShaderEntry {
  GLuint programID;
  std::vector<ShaderFile> files;
  void AddFile(const char *filename) {
    files.push_back();
    auto& file = files.back();
    file.path = filename;
    file.lastModifedTime = FileLastModifedTime(filename);
  }
};

struct ShaderTable {
  std::vector<ShaderEntry> entries;
} g_table;


char *ReadFileIntoMemory(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) return 0;
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  char *result = (char *)malloc(size);
  fread(result, 1, size, file);
  fclose(file);
};

ShaderType_Vertex, "test.vert", ShaderType_Fragment, "test.fragment"


GLuint VertexFragment(const char *vertex, const char *fragment) {
  

}





};




