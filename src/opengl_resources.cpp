void CreateIndexedVertex3DArray(GLuint *vertexArray, GLuint *vertexBuffer, GLuint *indexBuffer,
	U32 vertexCount, U32 indexCount, const Vertex3D *vertices, const U32 *indices, GLenum drawMode)
{
	glGenVertexArrays(1, vertexArray);
	glBindVertexArray(*vertexArray);

	glGenBuffers(1, vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex3D), vertices, drawMode);

	glGenBuffers(1, indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(U32), indices, drawMode);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (GLvoid*)offsetof(Vertex3D, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (GLvoid*)offsetof(Vertex3D, normal));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (GLvoid*)offsetof(Vertex3D, tangent));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (GLvoid*)offsetof(Vertex3D, texcoord));
	glBindVertexArray(0);
}

void CreateIndexedVertex2DArray(GLuint *vertexArray, GLuint *vertexBuffer, GLuint *indexBuffer,
	U32 vertexCount, U32 indexCount, const Vertex2D *vertices, const U32 *indices, GLenum drawMode)
{
	glGenVertexArrays(1, vertexArray);
	glBindVertexArray(*vertexArray);

	glGenBuffers(1, vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex2D), vertices, drawMode);

	glGenBuffers(1, indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(U32), indices, drawMode);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (GLvoid*)offsetof(Vertex2D, position));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (GLvoid*)offsetof(Vertex2D, texcoord));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (GLvoid*)offsetof(Vertex2D, color));
	glBindVertexArray(0);
}

void DeleteIndexedVertexArray(GLuint *vao, GLuint *vbo, GLuint *ebo)
{
	glDeleteBuffers(1, vao);
	glDeleteBuffers(1, vbo);
	glDeleteVertexArrays(1, vao);
}

GLuint CreateTextureWithMipmaps(const U8* pixels, U16 width, U16 height, GLenum internal_format, GLenum format, GLenum wrapMode, GLenum filterMode)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	int n = (int)log2((float)width);
	glTexStorage2D(GL_TEXTURE_2D, n + 1, internal_format, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}


GLuint CreateTextureWithoutMipmaps(const U8* pixels, U16 width, U16 height, GLenum internal_format, GLenum format, GLenum wrapMode, GLenum filterMode) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, 
    width, height, 0, format, GL_UNSIGNED_BYTE, pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);

	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}


GLuint CreateTextureArray(U16 width, U16 height, U32 depth, GLenum internal_format, GLenum wrap_mode, GLenum filter_mode)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internal_format, width, height, depth);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filter_mode); 
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filter_mode);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	return textureID;
}

MaterialDrawable CreateMaterialDrawable(MaterialData *data) {
	MaterialDrawable drawable = {};
	drawable.flags = data->flags;
	U8 *pixels_to_read = data->textureData;
	GLenum wrap_mode = GL_CLAMP_TO_EDGE;
	//if (data->flags & MaterialFlag_REPEAT) 
    wrap_mode = GL_REPEAT;

	if (data->flags & MaterialFlag_TRANSPARENT) {
		drawable.diffuse_texture_id = CreateTextureWithMipmaps(pixels_to_read,
      data->textureWidth, data->textureHeight, GL_RGBA8, GL_RGBA, wrap_mode);
		pixels_to_read += (data->textureWidth * data->textureHeight * 4);
	} else {
		drawable.diffuse_texture_id = CreateTextureWithMipmaps(pixels_to_read,
      data->textureWidth, data->textureHeight, GL_RGB8, GL_RGB, wrap_mode);
		pixels_to_read += (data->textureWidth * data->textureHeight * 3);
	}

	if (data->flags & MaterialFlag_NORMAL) {
		drawable.normal_texture_id = CreateTextureWithMipmaps(pixels_to_read,
      data->textureWidth, data->textureHeight, GL_RGB8, GL_RGB, wrap_mode);
		pixels_to_read += (data->textureWidth * data->textureHeight * 3);
	}

	if (data->flags & MaterialFlag_SPECULAR) {
		drawable.specular_texture_id = CreateTextureWithMipmaps(pixels_to_read,
      data->textureWidth, data->textureHeight, GL_RGB8, GL_RGB, wrap_mode);
	}

	return drawable;
}

void DestroyMaterialDrawable(MaterialDrawable* drawable) {
  if (drawable->diffuse_texture_id)
    glDeleteTextures(1, &drawable->diffuse_texture_id);
  if (drawable->specular_texture_id)
    glDeleteTextures(1, &drawable->specular_texture_id);
  if (drawable->normal_texture_id)
    glDeleteTextures(1, &drawable->normal_texture_id);
  drawable->flags = 0;
}
