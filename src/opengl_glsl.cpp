
enum ShaderType {
	ShaderType_VERTEX,
	ShaderType_FRAGMENT,
	ShaderType_GEOMETRY,
	ShaderType_COMPUTE,
	ShaderType_COUNT,
};

static const GLenum OPENGL_SHADER_TYPES[] = {
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER,
	GL_GEOMETRY_SHADER,
	GL_COMPUTE_SHADER,
};

static const char *SHADER_TYPE_STRINGS[] = {
	"Vertex Shader",
	"Fragment Shader",
	"Geometry Shader",
	"Compute Shader"
};

//TODO(Torin) Move this out into a utilty
//function and have the memory come from a 
//temporary memoryblock that the engine delegates
//specificaly for dynamic string operations
char* ReadFileIntoMemory(const char *filename) {
	//TODO(Torin) Make this function use temporary memory
	FILE* file = fopen(filename, "rb");
	if (file == NULL) return NULL;
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = (char*)malloc(file_size + 1);
	buffer[file_size] = 0;
	fread(buffer, 1, file_size, file);
	fclose(file);
	return buffer;
}

uint64_t string_to_uint64(const char *str, size_t length) {
	uint64_t result = 0;
	size_t scalar = 1;
  for (int64_t i = length - 1; i >= 0; i--) {
		assert(str[i] >= '0' && str[i] <= '9');
		uint64_t value = str[i] - '0';
		result += value * scalar;
		scalar *= 10;
	}
	return result;
}

#ifndef VENOM_RELEASE

struct ShaderParseEntry {
  U32 fileIndex;
  U32 lineCount;
};

struct ShaderParseInfo {
  static const U32 MAX_FILES = 8;
  static const U32 MAX_ENTRIES= 16;
  static const U32 MAX_FILENAME_LENGTH = 128;
  char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
  ShaderParseEntry entries[MAX_ENTRIES];
  U32 filenameCount;
  U32 entryCount;
};

static char* 
ReadAndParseShaderSource(const char* rootFilename, ShaderParseInfo* parseInfo) {
	FILE* rootFileHandle = fopen(rootFilename, "rb");
	if (rootFileHandle == 0) return 0;
	fseek(rootFileHandle , 0, SEEK_END);
	size_t rootFileSize = ftell(rootFileHandle);
	fseek(rootFileHandle, 0, SEEK_SET);
	char* rootFileBuffer = (char*)malloc(rootFileSize + 1);
	rootFileBuffer[rootFileSize] = 0;
	fread(rootFileBuffer, 1, rootFileSize, rootFileHandle);
	fclose(rootFileHandle);

#define match_string(at, string) match_text(at, string, static_strlen(string))
  auto match_text = [](const char *text, const char *match, size_t length) -> bool {
    for (size_t i = 0; i < length; i++) {
      if (text[i] != match[i] || text[i] == 0) 
        return false;
    }
    return true;
  };

  struct IncludedFile {
    char *filename;
    size_t filename_length;
    size_t file_size;
    char *start;
    char *end;
  } included_files[8];
  U32 files_to_include_count = 0;

  char *current = rootFileBuffer;
  while (*current != 0) {
    if (match_string(current, "#include")) {
      assert(files_to_include_count < ARRAY_COUNT(included_files));
      included_files[files_to_include_count].start = current;
      current += static_strlen("#include");
      while (*current != '<') current++;
      current = current + 1;
      included_files[files_to_include_count].filename = current;

      while (*current != '>')
        current++;

      included_files[files_to_include_count].filename_length = 
        current - included_files[files_to_include_count].filename;
      included_files[files_to_include_count].end = current + 1;
      files_to_include_count++;
    }
    current++;
  }

  size_t rootFilenameLength = strlen(rootFilename);
  assert(rootFilenameLength < ShaderParseInfo::MAX_FILENAME_LENGTH);
  memcpy(&parseInfo->filenames[1][0], rootFilename, rootFilenameLength);
  for (size_t i = 0; i < files_to_include_count; i++) {
    assert(included_files[i].filename_length < ShaderParseInfo::MAX_FILENAME_LENGTH);
    memcpy(&parseInfo->filenames[2+i][0], 
      included_files[i].filename, included_files[i].filename_length); 
  }

  static U64 totalHeaderLineCount = 0;
  if (totalHeaderLineCount == 0) {
    const char *current = GLSL_SHADER_HEADER;
    while (*current != 0) {
      if (*current == '\n') {
        totalHeaderLineCount++;
      }
      current++;
    }
  }


  auto GetBufferLineCount = [](const char* buffer) -> U64 {
    U64 result = 0;
    while (*buffer != 0) {
      if (*buffer == '\n')
        result++;
      buffer++;
    }
    return result;
  };

  ShaderParseEntry& headerEntry = parseInfo->entries[parseInfo->entryCount++];
  headerEntry.fileIndex = 0;
  headerEntry.lineCount = totalHeaderLineCount;
  
  if (files_to_include_count > 0) {
    size_t required_memory = rootFileSize;
    for(size_t i = 0; i < files_to_include_count; i++) {
      char file_to_open[1024];
      memcpy(file_to_open, VENOM_SHADER_FILE("include/"),
        static_strlen(VENOM_SHADER_FILE("include/")));
      memcpy(file_to_open + static_strlen(VENOM_SHADER_FILE("include/")),
        included_files[i].filename, included_files[i].filename_length);
      file_to_open[static_strlen(VENOM_SHADER_FILE("include/")) + 
        included_files[i].filename_length] = 0;

      FILE* file = fopen(file_to_open, "rb");
      if (file == NULL)  assert(false);
      fseek(file, 0, SEEK_END);
      included_files[i].file_size = ftell(file);
      required_memory += included_files[i].file_size;
      required_memory -= included_files[i].filename_length + 1;
      fclose(file);
    }
  
    char *result_source = (char*)malloc(required_memory + 1);
    result_source[required_memory] = 0;
    char *write_location = result_source;
    const char *mainFileLocation = rootFileBuffer;
    for (U32 i = 0; i < files_to_include_count; i++) {
      size_t lineCountToNextInclude = 0;
      size_t bytesToNextInclude = included_files[i].start - mainFileLocation;
      for (size_t n = 0; n < bytesToNextInclude; n++) {
        if (mainFileLocation[n] == '\n') lineCountToNextInclude++;
      }

      assert(parseInfo->entryCount + 1 < ARRAY_COUNT(parseInfo->entries));
      ShaderParseEntry& mainFileOffsetEntry = parseInfo->entries[parseInfo->entryCount++];
      mainFileOffsetEntry.fileIndex = 1;
      mainFileOffsetEntry.lineCount = lineCountToNextInclude;

      memcpy(write_location, mainFileLocation, bytesToNextInclude);
      mainFileLocation += bytesToNextInclude + 
        (included_files[i].end - included_files[i].start);
      write_location += bytesToNextInclude;

      char file_to_open[1024];
      memcpy(file_to_open, VENOM_SHADER_FILE("include/"),
        static_strlen(VENOM_SHADER_FILE("include/")));
      memcpy(file_to_open + static_strlen(VENOM_SHADER_FILE("include/")),
        included_files[i].filename, included_files[i].filename_length);
      file_to_open[static_strlen(VENOM_SHADER_FILE("include/")) + 
        included_files[i].filename_length] = 0;

      FILE* file = fopen(file_to_open, "rb");
      fread(write_location, 1, included_files[i].file_size, file);
      fclose(file);
      const char *current = write_location;
      write_location += included_files[i].file_size;
      
      size_t fileTotalLineCount = 0;
      while ((current - write_location) - included_files[i].file_size) {
        if (match_string(current, "#include")) {
          LOG_ERROR("No Support for nested #includes!");
        }
        if (*current == '\n')
          fileTotalLineCount++;
        current++;
      }

      assert(parseInfo->entryCount + 1 < ShaderParseInfo::MAX_ENTRIES);
      ShaderParseEntry& entry = parseInfo->entries[parseInfo->entryCount];
      entry.fileIndex = 2 + i;
      entry.lineCount = fileTotalLineCount;
      parseInfo->entryCount++;
    }

    size_t remainingBytesToWrite = 
      rootFileSize - (mainFileLocation - rootFileBuffer);
    memcpy(write_location, mainFileLocation, remainingBytesToWrite);
    write_location += remainingBytesToWrite;
    *write_location = 0;

    
    assert(parseInfo->entryCount + 1 < ShaderParseInfo::MAX_ENTRIES);
    ShaderParseEntry& entry = parseInfo->entries[parseInfo->entryCount++];
    entry.fileIndex = 1;
    entry.lineCount = GetBufferLineCount(mainFileLocation);
    free(rootFileBuffer);
    return result_source;
  } else {
    ShaderParseEntry& entry = parseInfo->entries[parseInfo->entryCount++];
    entry.fileIndex = 1;
    entry.lineCount = GetBufferLineCount(rootFileBuffer);
  }

  return rootFileBuffer;
}

#endif//!VENOM_RELEASE

#if 1
inline GLuint DEBUGCreateShaderProgramFromFiles(const char *filenames[4]) {
	assert(filenames[0] != nullptr && filenames[1] != nullptr);
	GLuint result = glCreateProgram();
	GLuint shaders[4] = {};
	for (int i = 0; i < 4; i++) {
		if (filenames[i] && strcmp(filenames[i], VENOM_SHADER_FILE("")) != 0) {

		  ShaderParseInfo parseInfo = {};	
      char *source = ReadAndParseShaderSource(filenames[i], &parseInfo);
			if (source == NULL) {
				LOG_ERROR("Failed to load shader source for file %s", filenames[i]);
				for (int n = i - 1; n > 0; n--)
					glDeleteShader(shaders[n]);
				return 0;
			}

      static char* generatedShaderConstants = 0;
      if (generatedShaderConstants == 0) {
        size_t requiredMemory = 0;
#define _(type,name,value) { \
  requiredMemory += static_strlen(#type);\
  requiredMemory += static_strlen(#name);\
  requiredMemory += 10; \
 }
        VenomShaderCalculatedValueList
#undef _

      generatedShaderConstants = (char*)malloc(requiredMemory);
      char* write = generatedShaderConstants;
#define _(type,name,value) { \
  int computedValue = (int)(value); \
  size_t bytesWritten = sprintf(write, "#define %s %d\n", #name, computedValue);  \
  write += bytesWritten; \
}
      VenomShaderCalculatedValueList
#undef _
      }


      FILE* debugOuputFile = fopen("generated_shader", "wb");
      assert(debugOuputFile != 0);
      fwrite(GLSL_SHADER_HEADER, 1, strlen(GLSL_SHADER_HEADER), debugOuputFile);
      fwrite(generatedShaderConstants, 1, strlen(generatedShaderConstants), debugOuputFile);
      fwrite(source, 1, strlen(source), debugOuputFile);
      fclose(debugOuputFile);


			shaders[i] = glCreateShader(OPENGL_SHADER_TYPES[i]);
			const char* shader_sources[3];
			shader_sources[0] = GLSL_SHADER_HEADER;
      shader_sources[1] = generatedShaderConstants;
			shader_sources[2] = source;
			glShaderSource(shaders[i], 3, shader_sources, NULL);
			glCompileShader(shaders[i]);

			GLint sucuess;
			glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &sucuess);
			if (sucuess == GL_FALSE) {
				char buffer[1024];
				glGetShaderInfoLog(shaders[i], 1024, NULL, buffer);
        U64 lineNumber = 0;
        const char *current = buffer;
        while(*current != 0) {
          if (*current == '(') {
            current++;
            if (isdigit(*current)) {
              const char *numberString = current;
              while(*current != ')') current++;
              size_t numberLength = current - numberString;
              lineNumber = string_to_uint64(numberString, numberLength);
              break;
            }
          }
          current++;
        }

        const char *filenameOfError = 0;
        U64 lineNumberOfError = lineNumber;
        U64 targetLineNumberOffset = lineNumber;
        U64 totalLineCountSoFar = 0;
        for (size_t i = 0; i < parseInfo.entryCount; i++) {
          totalLineCountSoFar += parseInfo.entries[i].lineCount;
          if (totalLineCountSoFar > targetLineNumberOffset) {
            U64 fileIndex = parseInfo.entries[i].fileIndex;
            filenameOfError = &parseInfo.filenames[fileIndex][0];
            if (i > 0) {
              U64 lastLineCount = totalLineCountSoFar - parseInfo.entries[i-1].lineCount;
              lineNumberOfError = lineNumber - lastLineCount;
            }
            break;
          }
        }

				LOG_ERROR("%s:%lu :: %s\n", filenameOfError, lineNumberOfError, buffer);
				glDeleteShader(shaders[i]);
			}
			else {
				glAttachShader(result, shaders[i]);
			}

			free(source);
		}
	}


	glLinkProgram(result);
	for (int i = 0; i < 4; i++)
	{
		if (shaders[i])	
		{
			glDetachShader(result, shaders[i]);
			glDeleteShader(shaders[i]);
		}
	}

#ifndef VENOM_RELEASE 
	GLint sucuess;
	glGetProgramiv(result, GL_LINK_STATUS, &sucuess);
	if (sucuess == GL_FALSE) {
		char buffer[1024];
		glGetProgramInfoLog(result, 1024, NULL, buffer);
		LOG_ERROR("Shader Program Link Error: %s\n", buffer);
		glDeleteProgram(result);
		return 0;
	}
#endif
	return result;
}
#endif

