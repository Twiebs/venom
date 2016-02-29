
enum ShaderType
{
	ShaderType_VERTEX,
	ShaderType_FRAGMENT,
	ShaderType_GEOMETRY,
	ShaderType_COMPUTE,
	ShaderType_COUNT,
};

static const GLenum OPENGL_SHADER_TYPES[] =
{
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER,
	GL_GEOMETRY_SHADER,
	GL_COMPUTE_SHADER,
};

static const char *SHADER_TYPE_STRINGS[] =
{
	"Vertex Shader",
	"Fragment Shader",
	"Geometry Shader",
	"Compute Shader"
};

//TODO(Torin) Move this out into a utilty
//function and have the memory come from a 
//temporary memoryblock that the engine delegates
//specificaly for dynamic string operations
char* ReadFileIntoMemory(const char *filename)
{
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

#ifndef VENOM_RELEASE
char *ReadAndParseShaderSource(const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (file == NULL) return NULL;
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = (char*)malloc(file_size + 1);
	buffer[file_size] = 0;
	fread(buffer, 1, file_size, file);
	fclose(file);

	if (buffer)
	{

		struct IncludedFile {
			char *filename;
			size_t filename_length;
			size_t file_size;
			char *start;
			char *end;
		} included_files[8];

		U32 files_to_include_count = 0;

		static auto match_text = [](char *text, const char *match, size_t length) -> bool
		{
			for (size_t i = 0; i < length; i++)
			{
				if (text[i] != match[i] || text[i] == 0) return false;
			}
			return true;
		};

#define match_string(at, string) match_text(at, string, static_strlen(string))

		char *current = buffer;
		while (*current != 0)
		{
			if (match_string(current, "#include")) {
				included_files[files_to_include_count].start = current;
				current += static_strlen("#include");
				while (*current != '<') current++;
				current = current + 1;
				included_files[files_to_include_count].filename = current;

				while (*current != '>')
					current++;

				included_files[files_to_include_count].filename_length = current - included_files[files_to_include_count].filename;
				included_files[files_to_include_count].end = current + 1;
				files_to_include_count++;
			}
			current++;
		}
	
		if (files_to_include_count > 0)
		{

			size_t required_memory = file_size;
			fori(files_to_include_count)
			{
				char file_to_open[1024];
				memcpy(file_to_open, VENOM_SHADER_FILE("include/"),
					static_strlen(VENOM_SHADER_FILE("include/")));
				memcpy(file_to_open + static_strlen(VENOM_SHADER_FILE("include/")),
					included_files[i].filename, included_files[i].filename_length);
				file_to_open[static_strlen(VENOM_SHADER_FILE("include/")) + included_files[i].filename_length] = 0;

				FILE* file = fopen(file_to_open, "rb");
				if (file == NULL)  assert(false);
				fseek(file, 0, SEEK_END);
				included_files[i].file_size = ftell(file);
				required_memory += included_files[i].file_size;
				fclose(file);

				required_memory -= included_files[i].filename_length + 1;
			}
		
			char *result_source = (char*)malloc(required_memory + 1);
			result_source[required_memory] = 0;
			char *write_location = result_source;
			char *read_location = buffer;

			for (U32 i = 0; i < files_to_include_count; i++)
			{
				size_t bytes_to_write = included_files[i].start - read_location;
				memcpy(write_location, read_location, bytes_to_write);
				read_location += bytes_to_write + (included_files[i].end - included_files[i].start);
				write_location += bytes_to_write;

				char file_to_open[1024];
				memcpy(file_to_open, VENOM_SHADER_FILE("include/"),
					static_strlen(VENOM_SHADER_FILE("include/")));
				memcpy(file_to_open + static_strlen(VENOM_SHADER_FILE("include/")),
					included_files[i].filename, included_files[i].filename_length);
				file_to_open[static_strlen(VENOM_SHADER_FILE("include/")) + included_files[i].filename_length] = 0;

				FILE* file = fopen(file_to_open, "rb");
				fread(write_location, 1, included_files[i].file_size, file);
				write_location += included_files[i].file_size;
			}

			size_t remaining_size_to_write = file_size - (read_location - buffer);
			memcpy(write_location, read_location, remaining_size_to_write);
			write_location += remaining_size_to_write;
			*write_location = 0;
		
			free(buffer);
			return result_source;
		}
		else
		{
			return buffer;
		}
	}

	return 0;
}
#endif


#if 1
inline GLuint DEBUGCreateShaderProgramFromFiles(const char *filenames[4])
{
	assert(filenames[0] != nullptr && filenames[1] != nullptr);
	GLuint result = glCreateProgram();
	GLuint shaders[4] = {};
	for (int i = 0; i < 4; i++)
	{
		if (filenames[i] && strcmp(filenames[i], VENOM_SHADER_FILE("")) != 0)
		{
			char *source = ReadAndParseShaderSource(filenames[i]);
			if (source == NULL)
			{
				LOG_ERROR("Failed to load shader source for file %s", filenames[i]);
				for (int n = i - 1; n > 0; n--)
				{
					glDeleteShader(shaders[n]);
				}
				return 0;
			}

			shaders[i] = glCreateShader(OPENGL_SHADER_TYPES[i]);

			const char* shader_sources[2];
			shader_sources[0] = GLSL_SHADER_HEADER;
			shader_sources[1] = source;
			glShaderSource(shaders[i], 2, shader_sources, NULL);
			glCompileShader(shaders[i]);

			GLint sucuess;
			glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &sucuess);
			if (sucuess == GL_FALSE) {
				char buffer[1024];
				glGetShaderInfoLog(shaders[i], 1024, NULL, buffer);
				LOG_ERROR("%s : %s : Compile Error: %s\n", SHADER_TYPE_STRINGS[i], filenames[i], buffer);
				glDeleteShader(shaders[i]);
			}
			else
			{
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

