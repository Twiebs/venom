
struct DEBUGVertex3D
{
	V3 postion;
	V4 color;
};

inline void PushBox(RenderGroup *group, const V3& pos, const V3& min, const V3& max, const V4& color)
{
	const DEBUGVertex3D vertices[] = 
	{
		//Front Face
		{ pos + V3(min.x, min.y, max.z),	color },
		{ pos + V3(max.x, min.y, max.z),	color },
		{ pos + V3(max.x, max.y, max.z),	color },
		{ pos + V3(min.x, max.y, max.z),	color },

		// Back face
		{ pos + V3(min.x, min.y, min.z),	color },
		{ pos + V3(min.x, max.y, min.z),	color },
		{ pos + V3(max.x, max.y, min.z),	color },
		{ pos + V3(max.x, min.y, min.z),	color },

		// Top face
		{ pos + V3(min.x, max.y, min.z),	color },
		{ pos + V3(min.x, max.y, max.z),	color },
		{ pos + V3(max.x, max.y, max.z),	color },
		{ pos + V3(max.x, max.y, min.z),	color },

		// Bottom face
		{ pos + V3(min.x, min.y, min.z),	color },
		{ pos + V3(max.x, min.y, min.z),	color },
		{ pos + V3(max.x, min.y, max.z),	color },
		{ pos + V3(min.x, min.y, max.z),	color },

		// Right face
		{ pos + V3(max.x, min.y, min.z),	color },
		{ pos + V3(max.x, max.y, min.z),	color },
		{ pos + V3(max.x, max.y, max.z),	color },
		{ pos + V3(max.x, min.y, max.z),	color },

		// Left face
		{ pos + V3(min.x, min.y, min.z),	color },
		//{ pos + V3(min.x, min.y, max.z),	color },
		{ pos + V3(min.x, max.y, max.z),	color },
		{ pos + V3(min.x, max.y, min.z),	color },
	};

	const U32 i = group->current_vertex_count;
	const U32 indices[] =
	{
		0 + i,  3 + i,  2 + i,
		0 + i,  2 + i,  1 + i,
		4 + i,  7 + i,  6 + i,
		4 + i,  6 + i,  5 + i,

		8 + i,  11 + i,  10 + i,
		8 + i,  10 + i, 9 + i,

		12 + i, 15 + i, 14 + i,
		12 + i, 14 + i, 13 + i,

		16 + i, 19 + i, 18 + i,
		16 + i, 18 + i, 17 + i,

		20 + i, 23 + i, 22 + i,
		20 + i, 22 + i, 21 + i,
	};

	PushArray(DEBUGVertex3D, ARRAY_COUNT(vertices), vertices, &group->vertexBlock);
	PushArray(U32, ARRAY_COUNT(indices), indices, &group->indexBlock);
	group->current_vertex_count += ARRAY_COUNT(vertices);
	group->current_index_count += ARRAY_COUNT(indices);
}

inline void PushSphere(RenderGroup *group, const V3& center, float radius)
{

}

inline void GeneratePlane()
{

}

inline void PushHexagon(RenderGroup *group, const V3& pos, float radius, const V4& color)
{
	static const V3 defualt_positions[] = 
	{
		V3(0.0f, 0.0f, 0.0f),
		V3(cos(DEG2RAD*0.0f), 0.0f, -sin(DEG2RAD*0.0f)),
		V3(cos(DEG2RAD*60.0f), 0.0f, -sin(DEG2RAD*60.0f)),
		V3(cos(DEG2RAD*120.0f), 0.0f, -sin(DEG2RAD*120.0f)),
		V3(cos(DEG2RAD*180.0f), 0.0f, -sin(DEG2RAD*180.0f)),
		V3(cos(DEG2RAD*240.0f), 0.0f, -sin(DEG2RAD*240.0f)),
		V3(cos(DEG2RAD*300.0f), 0.0f, -sin(DEG2RAD*300.0f)),
	};
#if 1
	DEBUGVertex3D vertices[7];
	for (size_t i = 0; i < 7; i++)
	{
		vertices[i].postion = (defualt_positions[i] * radius) + pos;
		vertices[i].color = color;
	}
#else
	DEBUGVertex3D vertices[14];
	for (size_t i = 0; i < 14; i++)
	{
		vertices[i].postion = defualt_positions[i] + pos;
		vertices[i].color = color;
	}
	for (size_t i = 7; i < 14; i++)
	{
		vertices[i].postion.y += 1;
	}

	U32 n = group->current_vertex_count;
	U32 indices[] =
	{
		0 + n, 2 + n, 1 + n,
		0 + n, 3 + n, 2 + n,
		0 + n, 4 + n, 3 + n,
		0 + n, 5 + n, 4 + n,
		0 + n, 6 + n, 5 + n,
		0 + n, 1 + n, 6 + n,
	};

#endif

	U32 n = group->current_vertex_count;

#if 1
	U32 indices[] =
	{
		0 + n, 2 + n, 1 + n,
		0 + n, 3 + n, 2 + n,
		0 + n, 4 + n, 3 + n,
		0 + n, 5 + n, 4 + n,
		0 + n, 6 + n, 5 + n,
		0 + n, 1 + n, 6 + n,
	};

#endif

#if 0
	U32 indices[18];
	for (size_t i = 0; i < 16; i+=3)
	{
		indices[i + 0] = n + 0;
		indices[i + 1] = n + i + 1;
		indices[i + 2] = n + i;
	}

	indices[15] = n;
	indices[16] = n + 2;
	indices[17] = n + 1;
#endif
	PushArray(DEBUGVertex3D, ARRAY_COUNT(vertices), vertices, &group->vertexBlock);
	PushArray(U32, ARRAY_COUNT(indices), indices, &group->indexBlock);
	group->current_vertex_count += ARRAY_COUNT(vertices);
	group->current_index_count += ARRAY_COUNT(indices);
}

inline void CalculateNormals(V3 *positions, V3* normals, U32 *indices, U32 vertexCount, U32 indexCount, uintptr_t stride)
{
	auto getpos = [&stride, &positions](U32 index)
	{
		return positions + (stride * index);
	};

	for (U32 i = 0; i < indexCount; i += 3)
	{
		auto index0 = indices[i + 0];
		auto index1 = indices[i + 1];
		auto index2 = indices[i + 2];

		auto edge0 = getpos(index1) - getpos(index0);
		auto edge1 = getpos(index2) - getpos(index0);
		auto normal = Cross(edge0, edge1);

		*(normals + (stride * index0)) += normal;
		*(normals + (stride * index1)) += normal;
		*(normals + (stride * index2)) += normal;
	}

	for (U32 i = 0; i < vertexCount; i++)
	{
		*(normals + (stride * i)) = Normalize(*(normals + (stride * i)));
	}
}


#if 0
inline void PushCube(RenderGroup *group, const V3& pos, const V4& color)
{
	const DEBUGVertex3D vertices[] = 
	{
		//Front Face
		{ pos + V3(-0.5f, -0.5f, 0.5f),	color },
		{ pos + V3(0.5f, -0.5f, 0.5f),	color },
		{ pos + V3(0.5f, 0.5f, 0.5f),	color },
		{ pos + V3(-0.5f, 0.5f, 0.5f),	color },

		// Back face
		{ pos + V3(-0.5f, -0.5f, -0.5f),color },
		{ pos + V3(-0.5f, 0.5f, -0.5f),	color },
		{ pos + V3(0.5f, 0.5f, -0.5f),	color },
		{ pos + V3(0.5f, -0.5f, -0.5f),	color },

		// Top face
		{ pos + V3(-0.5f, 0.5f, -0.5f),	color },
		{ pos + V3(-0.5f, 0.5f, 0.5f),	color },
		{ pos + V3(0.5f, 0.5f, 0.5f),	color },
		{ pos + V3(0.5f, 0.5f, -0.5f),	color },

		// Bottom face
		{ pos + V3(-0.5f, -0.5f, -0.5f),color },
		{ pos + V3(0.5f, -0.5f, -0.5f),	color },
		{ pos + V3(0.5f, -0.5f, 0.5f),	color },
		{ pos + V3(-0.5f, -0.5f, 0.5f),	color },

		// Right face
		{ pos + V3(0.5f, -0.5f, -0.5f),	color },
		{ pos + V3(0.5f, 0.5f, -0.5f),	color },
		{ pos + V3(0.5f, 0.5f, 0.5f),	color },
		{ pos + V3(0.5f, -0.5f, 0.5f),	color },

		// Left face
		{ pos + V3(-0.5f, -0.5f, -0.5f),color },
		{ pos + V3(-0.5f, -0.5f, 0.5f),	color },
		{ pos + V3(-0.5f, 0.5f, 0.5f),	color },
		{ pos + V3(-0.5f, 0.5f, -0.5f),	color },
	};

	const U32 i = group->current_vertex_count;
	const U32 indices[] =
	{
		0 + i,  3 + i,  2 + i,
		0 + i,  2 + i,  1 + i,
		4 + i,  7 + i,  6 + i,
		4 + i,  6 + i,  5 + i,

		8 + i,  11 + i,  10 + i,
		8 + i,  10 + i, 9 + i,

		12 + i, 15 + i, 14 + i,
		12 + i, 14 + i, 13 + i,

		16 + i, 19 + i, 18 + i,
		16 + i, 18 + i, 17 + i,

		20 + i, 23 + i, 22 + i,
		20 + i, 22 + i, 21 + i,
	};

	PushArray(DEBUGVertex3D, ArrayCount(vertices), vertices, &group->vertexBlock);
	PushArray(U32, ArrayCount(indices), indices, &group->indexBlock);
	group->current_vertex_count += ArrayCount(vertices);
	group->current_index_count += ArrayCount(indices);
}
#endif

inline void PushLine(RenderGroup *group, const V3& from, const V3& to, const V4& color)
{
	static const U32 INDICES[2]{ 0, 1 };
	float vertices[] = { from.x, from.y, from.z, color.x, color.y, color.z, color.w,
							to.x, to.y, to.z, color.x, color.y, color.z, color.w };	
	PushArray(DEBUGVertex3D, 2, vertices, &group->vertexBlock);
	PushArray(U32, 2, INDICES, &group->indexBlock);
}

#if 1
#endif
