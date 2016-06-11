void InitGBuffer(GBuffer* gbuffer, const U32 viewportWidth, const U32 viewportHeight) 
{
  glGenFramebuffers(1, &gbuffer->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->framebuffer);
  
  glGenTextures(1, &gbuffer->positionDepth);
  glBindTexture(GL_TEXTURE_2D, gbuffer->positionDepth);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &gbuffer->normal);
  glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  glGenTextures(1, &gbuffer->albedoSpecular);
  glBindTexture(GL_TEXTURE_2D, gbuffer->albedoSpecular);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &gbuffer->depthTexture);
  glBindTexture(GL_TEXTURE_2D, gbuffer->depthTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, viewportWidth, viewportHeight);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
    GL_TEXTURE_2D, gbuffer->positionDepth, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 
    GL_TEXTURE_2D, gbuffer->normal, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, 
    GL_TEXTURE_2D, gbuffer->albedoSpecular, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
    GL_TEXTURE_2D, gbuffer->depthTexture, 0);

  GLuint attachments[] = { 
    GL_COLOR_ATTACHMENT0, 
    GL_COLOR_ATTACHMENT1, 
    GL_COLOR_ATTACHMENT2,
  };

  glDrawBuffers(3, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void InitCascadedShadowMaps(CascadedShadowMap* csm, 
  F32 viewportWidth, F32 viewportHeight, F32 fieldOfView) 
{ 
  glGenFramebuffers(1, &csm->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, csm->framebuffer);
  glDrawBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenTextures(1, &csm->depthTexture);
  glBindTexture(GL_TEXTURE_2D_ARRAY, csm->depthTexture);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
               SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, 
               SHADOW_MAP_CASCADE_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
    csm->cascadeFrustums[i].aspect_ratio = 
      viewportWidth / viewportHeight;
    csm->cascadeFrustums[i].field_of_view = fieldOfView + 0.2f;
  }
}

void InitOmnidirectionalShadowMap(OmnidirectionalShadowMap* osm) 
{
  glGenTextures(1, &osm->depthCubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, osm->depthCubemap);
  for (size_t i = 0; i < 6; i++)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
      OMNI_SHADOW_MAP_RESOLUTION, OMNI_SHADOW_MAP_RESOLUTION, 0, 
      GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
}

inline void
SSAOInit(SSAO* ssao, U32 viewportWidth, U32 viewportHeight) 
{
  glGenFramebuffers(1, &ssao->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, ssao->framebuffer);
  
  glGenTextures(1, &ssao->bufferTexture);
  glBindTexture(GL_TEXTURE_2D, ssao->bufferTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
    GL_TEXTURE_2D, ssao->bufferTexture, 0);

  GLuint attachments[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  RNGSeed seed(7);
  V3 ssaoNoise[SSAO_NOISE_SIZE * SSAO_NOISE_SIZE];
  for (size_t i = 0; i < SSAO_NOISE_SIZE * SSAO_NOISE_SIZE; i++) {
    ssaoNoise[i].x = RandomInRange(-1.0f, 1.0f, seed);
    ssaoNoise[i].y = RandomInRange(-1.0f, 1.0f, seed);
    ssaoNoise[i].z = 0.0f;
  }

  glGenTextures(1, &ssao->noiseTexture);
  glBindTexture(GL_TEXTURE_2D, ssao->noiseTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SSAO_NOISE_SIZE, SSAO_NOISE_SIZE,
    0, GL_RGB, GL_FLOAT, ssaoNoise);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  

  V3 sample;
  for (size_t i = 0; i < SSAO_SAMPLE_COUNT; i++) {
    sample.x = RandomInRange(-1.0, 1.0, seed);
    sample.y = RandomInRange(-1.0, 1.0, seed);
    sample.z = RandomInRange(-1.0, 1.0, seed);
    sample = Normalize(sample);
    ssao->kernelSamples[i] = sample;
  }
}

static inline
void CreateIndexedVertex1PArray(
  GLuint* vao, GLuint* vbo, GLuint* ebo,
  const U32 vertexCount, const U32 indexCount, 
  const GLenum drawMode,
  const V3* vertices = 0, const U32* indices = 0)
{
  glGenVertexArrays(1, vao);
  glBindVertexArray(*vao);
  
  glGenBuffers(1, vbo);
  glBindBuffer(GL_ARRAY_BUFFER, *vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(V3) * vertexCount, vertices, drawMode);
  
  glGenBuffers(1, ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(U32) * indexCount, indices, drawMode);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  glBindVertexArray(0);
}


static inline
void CreateDebugRenderResources(DebugRenderResources* resources) 
{

//NOTE(Torin) Contains definitions for debug primitave vertices and indices
#include "render_debug_shapes.h"

  glGenVertexArrays(1, &resources->vao);
  glBindVertexArray(resources->vao);

#define ShapeList \
  _(cube) _(sphere) _(axis)
  
  glGenBuffers(1, &resources->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, resources->vbo);
#define _(name) + sizeof(name##Vertices)
  glBufferData(GL_ARRAY_BUFFER, 0 ShapeList, 0, GL_DYNAMIC_DRAW);
#undef _


  {
  size_t currentOffset = 0;
    #define _(name) glBufferSubData(GL_ARRAY_BUFFER, currentOffset, \
      sizeof(name##Vertices), name##Vertices); \
      currentOffset += sizeof(name##Vertices);
    ShapeList
    #undef _
  }



  glGenBuffers(1, &resources->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources->ebo);
  #define _(name) + sizeof(name##Indices)
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0 + ShapeList, 0, GL_DYNAMIC_DRAW);
  #undef _

  {
    size_t currentOffset = 0;
    #define _(name) glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, currentOffset, \
      sizeof(name##Indices), name##Indices); \
    currentOffset += sizeof(name##Indices);
    ShapeList
    #undef _
  }

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glBindVertexArray(0);

  {
    size_t currentOffset = 0;
    #define _(name) resources->name##IndexCount = ARRAY_COUNT(name##Indices); \
      resources->name##IndexOffset = currentOffset; \
      currentOffset += sizeof(name##Indices);
    ShapeList
    #undef _
  }
}



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

void DestroyIndexedVertexArray(GLuint *vao, GLuint *vbo, GLuint *ebo) {
	glDeleteBuffers(1, vbo);
	glDeleteBuffers(1, ebo);
	glDeleteVertexArrays(1, vao);
}

inline void 
DestroyIndexedVertexArray(IndexedVertexArray* array){
  DestroyIndexedVertexArray(&array->vertexArrayID,
    &array->vertexBufferID, &array->indexBufferID);
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
  assert(data->flags & MaterialFlag_DIFFUSE);
	U8 *pixels_to_read = data->textureData;
	GLenum wrap_mode = GL_CLAMP_TO_EDGE;
	if(data->flags & MaterialFlag_REPEAT) 
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


inline RenderGroup CreateDebugRenderGroup() {
	RenderGroup result = {};

	glGenVertexArrays(1, &result.vao);
	glGenBuffers(1, &result.vbo);
	glGenBuffers(1, &result.ebo);

	glBindVertexArray(result.vao);
	glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V3) + sizeof(V4), (GLvoid*)0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V3) + sizeof(V4), (GLvoid*)sizeof(V3));
	return result;
}

inline RenderGroup CreateImGuiRenderGroup() {
	RenderGroup result = {};
	glGenVertexArrays(1, &result.vao);
	glGenBuffers(1, &result.vbo);
	glGenBuffers(1, &result.ebo);

	glBindVertexArray(result.vao);
	glBindBuffer(GL_ARRAY_BUFFER, result.vbo);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));
	glBindVertexArray(0);
	return result;
}

inline void FreeRenderGroup(RenderGroup *group) {
	glDeleteBuffers(1, &group->vao);
	glDeleteBuffers(1, &group->ebo);
	glDeleteVertexArrays(1, &group->vao);
}


