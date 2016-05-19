
void APIENTRY OpenGLDebugProc(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void *userdata)
{
	static const char* OPENGL_SOURCE_STRINGS[] =
	{
		"GL_DEBUG_SOURCE_API",
		"GL_DEBUG_SOURCE_WINDOW_SYSTEM",
		"GL_DEBUG_SOURCE_SHADER_COMPILER",
		"GL_DEBUG_SOURCE_THIRD_PARTY",
		"GL_DEBUG_SOURCE_APPLICATION",
		"GL_DEBUG_SOURCE_OTHER",
	};

	static const char* OPENGL_ERROR_TYPE_STRINGS[] =
	{
		"GL_DEBUG_TYPE_ERROR",
		"GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR",
		"GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR",
		"GL_DEBUG_TYPE_PORTABILITY",
		"GL_DEBUG_TYPE_PERFORMANCE",
		"GL_DEBUG_TYPE_MARKER",
		"GL_DEBUG_TYPE_PUSH_GROUP",
		"GL_DEBUG_TYPE_POP_GROUP",
		"GL_DEBUG_TYPE_OTHER",
	};

	static const char* OPENGL_SEVERITY_STRINGS[] =
	{
		"GL_DEBUG_SEVERITY_HIGH",
		"GL_DEBUG_SEVERITY_MEDIUM",
		"GL_DEBUG_SEVERITY_LOW",
		"GL_DEBUG_SEVERITY_NOTIFICATION"
	};

	DebugLog *log = (DebugLog*)userdata;
	fori(log->logged_opengl_id_count)
		if (log->logged_opengl_ids[i] == id) return;
	log->logged_opengl_ids[log->logged_opengl_id_count++] = id;
	assert(log->logged_opengl_id_count < DebugLog::OPENGL_ID_COUNT_MAX);

	const char *source_string = OPENGL_SOURCE_STRINGS[source - GL_DEBUG_SOURCE_API];
	const char *type_string = OPENGL_ERROR_TYPE_STRINGS[type - GL_DEBUG_TYPE_ERROR];

	// NOTE(Torin) Kronos decided to not put GL_DEBUG_SEVERITY_NOTIFCATION
	// Next to the other values for some reason...
	int severity_string_index = severity - GL_DEBUG_SEVERITY_HIGH;
	if (severity_string_index < 0) severity_string_index = 3;
	const char *severity_string = OPENGL_SEVERITY_STRINGS[severity_string_index];

	LOG_ERROR("[OpenGL Debug Message]\n"
		"Source: %s\n"
		"Type: %s\n"
		"Severity: %s\n"
		"ID: %u\n"
		"Message: %s\n"
		"\n",
		source_string,
		type_string,
		severity_string,
		id, message);
}

inline void OpenGLEnableDebug(DebugLog *log) {
	glDebugMessageCallback(OpenGLDebugProc, log);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
}
