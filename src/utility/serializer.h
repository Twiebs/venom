
//Venom Serializer
namespace vs {
  int BeginFileWrite(const char *filename);
  void EndFileWrite();
  void BeginGroupWrite(const char *name);
  void EndGroupWrite(); 
  void WriteString(const char *name, const char *value, size_t length = 0);
  void WriteFloat32(const char *name, uint32_t count, void *values);

  int BeginFileRead(const char *filename);
  void EndFileRead();
  int BeginGroupRead(); 
  void EndGroupRead(); 
  int ReadString(const char *name, char *outBuffer, size_t outBufferSize); 
  void ReadFloat32(const char *name, uint32_t count, void *outBuffer);
}//namespace vs


