#pragma once

inline RenderGroup CreateDebugRenderGroup()
{
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

inline RenderGroup CreateImGuiRenderGroup()
{
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

inline void FreeRenderGroup(RenderGroup *group)
{
	glDeleteBuffers(1, &group->vao);
	glDeleteBuffers(1, &group->ebo);
	glDeleteVertexArrays(1, &group->vao);
}

inline void InitializeCamera(Camera *camera, float fov, float near_clip, float far_clip)
{
	camera->position = V3(0.0f, 0.0f, 0.0f);
	camera->front = V3(0.0f, 0.0f, 0.0f);
	camera->fov = fov;
	camera->yaw = 0.0f;
	camera->pitch = 0.0f;
	camera->near_clip = near_clip;
	camera->far_clip = far_clip;
}

inline void UpdateCamera(Camera *camera)
{
	camera->front.x = cosf(camera->yaw) * cosf(camera->pitch);
	camera->front.y = sinf(camera->pitch);
	camera->front.z = sin(camera->yaw) * cos(camera->pitch);
	camera->front = Normalize(camera->front);
}

inline M4 CreateViewMatrix(Camera *camera)
{
	M4 result = LookAt(camera->position, camera->position + camera->front, V3(0.0f, 1.0f, 0.0f));
	return result;
}

#if 0
inline void PushStreamDraw(RenderGroup *group, const void *vertices, const U32 *indices, size_t vertices_size, size_t index_count)
{
	glBindVertexArray(group->vao);
	glBindBuffer(GL_ARRAY_BUFFER, group->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STREAM_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, group->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(U32), indices, GL_STREAM_DRAW);
	group->current_index_count += index_count;
}

#define FlushVertexBuffer(vertexType, renderGroup) 
#define FlushIndexBuffer(indexType, renderGroup)

inline void InternalFlushIndexBuffer(RenderGroup *group, size_t elementSize)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, group->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementSize * group->current_index_count, group->indexBlock.base, GL_DYNAMIC_DRAW);
}

inline void InternalFlushVertexBuffer(RenderGroup *group, size_t vertexTypeSize)
{

	glBindBuffer(GL_ARRAY_BUFFER, group->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexTypeSize * group->current_vertex_count, group->vertexBlock.base, GL_DYNAMIC_DRAW);
}
#endif