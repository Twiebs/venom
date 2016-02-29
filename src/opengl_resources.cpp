
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


GLuint CreateTextureWithoutMipmaps(const U8* pixels, U16 width, U16 height, GLenum internal_format, GLenum format, GLenum wrapMode, GLenum filterMode)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);

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



//TODO(Torin) What the fuck?

#if 0
GLuint CreateMaterial(const MaterialData *data)
{
	GLuint textureArrayID;
	glGenTextures(1, &textureArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayID);
	//glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB, data->textureWidth, data->textureHeight, 3);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, data->textureWidth, data->textureHeight, 3, 0, GL_RGB, GL_UNSIGNED_BYTE, data->textureData);
	//glTexImage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB, data->textureWidth, data->textureHeight, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, data->textureData);
	//glTexImage3D(GL_TEXTURE_2D_ARRAY, 2, GL_RGB, data->textureWidth, data->textureHeight, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, data->textureData);	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return textureArrayID;
}
#endif