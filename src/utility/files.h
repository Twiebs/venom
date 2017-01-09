
char* ReadFileIntoMemoryAndNullTerminate(const char *filename) {
  //TODO(Torin) Make this function use temporary memory
  FILE* file = fopen(filename, "rb");
  if (file == NULL) return NULL;
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* buffer = (char*)MemoryAllocate(file_size + 1);
  buffer[file_size] = 0;
  fread(buffer, 1, file_size, file);
  fclose(file);
  return buffer;
}