
//Venom Serializer
namespace vs {

enum PropertyType {
  PropertyType_Invalid,
  PropertyType_FileTag,
  PropertyType_Group,
  PropertyType_Int,
  PropertyType_Float,
  PropertyType_Char,
};

struct PropertyHeader {
  uint32_t type;        
  uint32_t nameLength; 
  uint32_t nameOffset;
  uint32_t valueCount;
  uint32_t valueOffset;
} __attribute((packed));

struct Context {
  FILE* file;

  PropertyHeader *headerBuffer;
  uint64_t headerBufferCount;
  uint64_t headerBufferCapacity;

  uint8_t *dataBuffer;
  uint64_t dataBufferSize;
  uint64_t dataBufferUsed;
  
  PropertyHeader *activeHeader; 
  uint32_t expectedPropertyIndex;
} ctx;

uint32_t VERIFICATION_NUMBER = ('F' << 24) | ('F' << 16) | ('M' << 8) | ('V');

static inline
uint32_t PushData(const void *data, const size_t size){
  assert(ctx.dataBufferUsed + size < ctx.dataBufferSize);
  assert(ctx.dataBufferUsed + size < ((uint64_t)1 << 32));
  memcpy(&ctx.dataBuffer[ctx.dataBufferUsed], data, size);
  uint32_t result = ctx.dataBufferUsed;
  ctx.dataBufferUsed += size;
  return result;
}

static inline
PropertyHeader *PushHeader(PropertyType type, const char *name){
  assert(ctx.headerBufferCount + 1 < ctx.headerBufferCapacity);
  PropertyHeader *result = &ctx.headerBuffer[ctx.headerBufferCount++];
  result->type = type;
  result->nameLength = strlen(name);
  result->nameOffset = PushData(name, result->nameLength); 
  return result;
}

static inline
int StringsAreEqual(const char *a, size_t lengthA, const char *b, size_t lengthB){
  if(lengthA != lengthB) return 0;
  for(size_t i = 0; i < lengthA; i++)
    if(a[i] != b[i]) return 0;
  return 1;
}

static inline
int OpenFile(const char *filename, const char *mode){
  assert(ctx.file == 0);
  ctx.file = fopen(filename, mode);
  if(ctx.file == 0) return 0;
  ctx.headerBufferCapacity = 1024;
  ctx.headerBuffer = (PropertyHeader *)
    malloc(ctx.headerBufferCapacity * sizeof(PropertyHeader));
  ctx.headerBufferCount = 0;
  ctx.dataBufferSize = 1024*1024*8;
  ctx.dataBuffer = (uint8_t *)malloc(ctx.dataBufferSize);
  ctx.dataBufferUsed = 0;
  return 1;
}

static inline
void CloseFile() {
  assert(ctx.file != 0);
  assert(ctx.activeHeader == 0);
  free(ctx.headerBuffer);
  free(ctx.dataBuffer);
  ctx.headerBuffer = 0;
  ctx.dataBuffer = 0;
  fclose(ctx.file);
  ctx.file = 0;
}

static inline
int IsValidFileTag(PropertyHeader *header){
  if(header->type != PropertyType_FileTag) return 0;
  if(header->nameLength != VERIFICATION_NUMBER) return 0;
  return 1;
}

//TODO(Torin) Consider changing value count to value size
//Returns the bytes written to the outbuffer on sucuess
//Returns  0 if the outbuffer is to small to hold the value
//Returns -1 if there is no matching property with that name
//Returns -2 if the property was found but the types do not match
static inline
int ReadData(const char *name, PropertyType type, uint32_t expectedCount, void *outBuffer, size_t dataSize){
  assert(ctx.headerBufferCount > 0);
  uint32_t totalHeadersTested = 0, currentHeaderIndex = ctx.expectedPropertyIndex;
  while(totalHeadersTested < ctx.headerBufferCount) {
    PropertyHeader *header = ctx.headerBuffer + currentHeaderIndex;
    const char *headerName = (const char *)ctx.dataBuffer + header->nameOffset;
    if(StringsAreEqual(headerName, header->nameLength, name, strlen(name))){
      if(header->type != type) return -2;
      if(header->valueCount > expectedCount) return 0;
      uint8_t *values = ctx.dataBuffer + header->valueOffset;
      memcpy(outBuffer, values, header->valueCount * dataSize); 
      ctx.expectedPropertyIndex = (currentHeaderIndex + 1) % ctx.headerBufferCount;
      return header->valueCount;
    }
    currentHeaderIndex += 1;
    currentHeaderIndex = currentHeaderIndex % ctx.headerBufferCount;
    totalHeadersTested += 1;
  }
  return -1;
}

static inline
void ReadDataAndReportErrors(const char *name, PropertyType type, uint32_t expectedCount, void *outBuffer, size_t dataSize) {
  int result = ReadData(name, type, expectedCount, outBuffer, dataSize);
  if(result > 0) return;
  else if(result == 0) { LogError("Not enough provided storage to read data"); }
  else if(result == -1) { LogError("Property %s is missing", name); }
  else if(result == -2) { LogError("Property %s is not expected type", name); }
}

int BeginFileWrite(const char *filename) {
  if(OpenFile(filename, "wb") != 0){
    PropertyHeader header = {};
    header.type = PropertyType_FileTag;
    header.nameLength = VERIFICATION_NUMBER;
    fwrite(&header, sizeof(header), 1, ctx.file);
    return 1;
  }
  return 0;
}

void EndFileWrite() {
  PropertyHeader header = {};
  header.type = PropertyType_FileTag;
  header.nameLength = VERIFICATION_NUMBER;
  fwrite(&header, sizeof(header), 1, ctx.file);
  CloseFile();
}


void BeginGroupWrite(const char *name) {
  assert(ctx.activeHeader == 0);
  ctx.activeHeader = PushHeader(PropertyType_Group, name);
}

void EndGroupWrite() {
  assert(ctx.activeHeader != 0);
  ctx.activeHeader->valueOffset = ctx.headerBufferCount - 1;
  ctx.activeHeader->valueCount = ctx.dataBufferUsed;
  fwrite(ctx.headerBuffer, sizeof(PropertyHeader), ctx.headerBufferCount, ctx.file);
  fwrite(ctx.dataBuffer, 1, ctx.dataBufferUsed, ctx.file);
  ctx.headerBufferCount = 0;
  ctx.dataBufferUsed = 0;
  ctx.activeHeader = 0;
  ctx.expectedPropertyIndex = 0;
}

void WriteString(const char *name, const char *value, size_t length){
  PropertyHeader *header = PushHeader(PropertyType_Char, name);
  if(length == 0) length = strlen(value);
  header->valueCount = length; 
  header->valueOffset = PushData(value, header->valueCount);
}

void WriteFloat32(const char *name, uint32_t count, void *values) {
  PropertyHeader *header = PushHeader(PropertyType_Float, name);
  header->valueCount = count;
  header->valueOffset = PushData(values, count*sizeof(float));
}

int BeginFileRead(const char *filename) {
  if(OpenFile(filename, "rb") == 0) return 0;
  PropertyHeader header = {};
  fread(&header, sizeof(header), 1, ctx.file);
  return IsValidFileTag(&header);
}

void EndFileRead() {
  CloseFile();
}

int BeginGroupRead() {
  PropertyHeader header = {};
  fread(&header, sizeof(header), 1, ctx.file);
  if(header.type != PropertyType_Group) return 0;
  assert(header.valueOffset < ctx.headerBufferCapacity);
  
  ctx.dataBufferSize = header.valueCount;
  ctx.headerBufferCount = header.valueOffset;
  fread(ctx.headerBuffer, sizeof(PropertyHeader), header.valueOffset, ctx.file);
  fread(ctx.dataBuffer, 1, header.valueCount, ctx.file);
  return 1;
}

void EndGroupRead() {
  ctx.dataBufferSize = 0;
  ctx.headerBufferCount = 0;
  ctx.activeHeader = 0;
}

int ReadString(const char *name, char *outBuffer, size_t outBufferSize) {
  assert(ctx.headerBufferCount > 0);
  uint32_t totalHeadersTested = 0; 
  uint32_t currentHeaderIndex = ctx.expectedPropertyIndex;
  while(totalHeadersTested < ctx.headerBufferCount) {
    PropertyHeader *header = ctx.headerBuffer + currentHeaderIndex;
    const char *headerName = (const char *)ctx.dataBuffer + header->nameOffset;
    if(StringsAreEqual(headerName, header->nameLength, name, strlen(name))){
      const char *value = (const char *)ctx.dataBuffer + header->valueOffset;
      if(header->valueCount > outBufferSize + 1) return 0;
      memcpy(outBuffer, value, header->valueCount);
      outBuffer[header->valueCount] = 0;
      ctx.expectedPropertyIndex = (currentHeaderIndex + 1) % ctx.headerBufferCount;
      return header->valueCount + 1;
    }
    currentHeaderIndex += 1;    
    currentHeaderIndex = currentHeaderIndex % ctx.headerBufferCount;
    totalHeadersTested += 1;
  }

  return -1;
}


void ReadFloat32(const char *name, uint32_t count, void *outBuffer){
  ReadDataAndReportErrors(name, PropertyType_Float, count, outBuffer, sizeof(float));
}


}//namespace vs


