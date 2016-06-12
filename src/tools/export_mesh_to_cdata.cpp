#include <stdio.h>
#include "../venom_platform.h"
#include "../offline_asset_tools.cpp"

int main(int argc, const char **argv) {
  if (argc < 3) {
    printf("must provide an input mesh and output file\n");
    return 1;
  }

  const char *inputMesh = argv[2];
  const char *outputFile = argv[3];

  auto data = ImportExternalModelData(inputMesh, 0);
  FILE* file = fopen(outputFile, "wb");
  if (file == 0) {
    printf("failed to open outputfile %s", outputFile);
  }
  
  for (size_t i = 0; i < data.meshData.vertexCount; i++) {
    fprintf(file, "V3{%ff, %ff, %ff},\n", data.meshData.vertices[i].position.x,
      data.meshData.vertices[i].position.y, data.meshData.vertices[i].position.z);
  }

  fprintf(file, "\n");
  for (size_t i = 0; i < data.meshData.indexCount; i++) {
    fprintf(file, "%u,", data.meshData.indices[i]);
    if (i % 3 == 0) fprintf(file, "\n");
  }

  return 0;
}