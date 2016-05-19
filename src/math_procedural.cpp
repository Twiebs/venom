
void GetSubdiviedCubeVertexAndIndexCount(unsigned int cellsPerEdge, 
	unsigned int *vertexCount, unsigned int *indexCount)
{
	*vertexCount = (cellsPerEdge + 1) * (cellsPerEdge + 1);
	*indexCount = (cellsPerEdge * cellsPerEdge) * 6;
	*vertexCount *= 6;
	*indexCount *= 6;
}

inline void SetPlaneIndicesClockwise(U32 cellsPerEdge, U32 *indices, U32 indexOffset)
{
	U32 index = 0;
	for (U32 y = 0; y < cellsPerEdge; y++) {
		for (U32 x = 0; x < cellsPerEdge; x++) {
			indices[index++] = ((y + 0) * (cellsPerEdge + 1)) + (x + 0) + indexOffset;
			indices[index++] = ((y + 1) * (cellsPerEdge + 1)) + (x + 0) + indexOffset;
			indices[index++] = ((y + 1) * (cellsPerEdge + 1)) + (x + 1) + indexOffset;
			indices[index++] = ((y + 0) * (cellsPerEdge + 1)) + (x + 0) + indexOffset;
			indices[index++] = ((y + 1) * (cellsPerEdge + 1)) + (x + 1) + indexOffset;
			indices[index++] = ((y + 0) * (cellsPerEdge + 1)) + (x + 1) + indexOffset;
		}
	}
}

inline void SetPlaneIndicesCounterClockwise(U32 cellsPerEdge, U32 *indices, U32 indexOffset)
{
	U32 index = 0;
	for (U32 y = 0; y < cellsPerEdge; y++) {
		for (U32 x = 0; x < cellsPerEdge; x++) {
			indices[index++] = ((y + 0) * (cellsPerEdge + 1)) + (x + 0) + indexOffset;
			indices[index++] = ((y + 0) * (cellsPerEdge + 1)) + (x + 1) + indexOffset;
			indices[index++] = ((y + 1) * (cellsPerEdge + 1)) + (x + 1) + indexOffset;
			indices[index++] = ((y + 0) * (cellsPerEdge + 1)) + (x + 0) + indexOffset;
			indices[index++] = ((y + 1) * (cellsPerEdge + 1)) + (x + 1) + indexOffset;
			indices[index++] = ((y + 1) * (cellsPerEdge + 1)) + (x + 0) + indexOffset;
		}
	}
}

void CalculateVertexTangents(Vertex3D* vertices, const U32* indices,
    const U32 vertexCount, const U32 indexCount) {

  for (size_t i = 0; i < indexCount; i+=3) {
    U32 indexA = indices[i+0];
    U32 indexB = indices[i+1];
    U32 indexC = indices[i+2];
    const V3& v1 = vertices[indexA].position;
    const V3& v2 = vertices[indexB].position;
    const V3& v3 = vertices[indexC].position;
    const V2& w1 = vertices[indexA].texcoord;
    const V2& w2 = vertices[indexB].texcoord;
    const V2& w3 = vertices[indexC].texcoord;

    F32 x1 = v2.x - v1.x;
    F32 x2 = v3.x - v1.x;
    F32 y1 = v2.y - v1.y;
    F32 y2 = v3.y - v1.y;
    F32 z1 = v2.z - v1.z;
    F32 z2 = v3.z - v1.z;

    F32 s1 = w2.x - w1.x;
    F32 s2 = w3.x - w1.x;
    F32 t1 = w2.y - w1.y;
    F32 t2 = w3.y - w1.y;

    F32 r = 1.0F / (s1 * t2 - s2 * t1);
    V3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
    V3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

    vertices[indexA].tangent += sdir;
    vertices[indexB].tangent += sdir;
    vertices[indexC].tangent += tdir;
  }

  for (size_t i = 0; i < vertexCount; i++)
    vertices[i].tangent = Normalize(vertices[i].tangent);
}

void CalculateSurfaceNormals (Vertex3D* vertices, const U32 vertexCount,  
                              U32* indices, const U32 indexCount) 
{
  for (U32 i = 0; i < indexCount; i += 3) {
    auto index0 = indices[i + 0];
    auto index1 = indices[i + 1];
    auto index2 = indices[i + 2];

    auto edge0 = vertices[index1].position - vertices[index0].position;
    auto edge1 = vertices[index2].position - vertices[index0].position;
    auto normal = Cross(edge0, edge1);

    vertices[index0].normal += normal;
    vertices[index1].normal += normal;
    vertices[index2].normal += normal;
  }

  for (U32 i = 0; i < vertexCount; i++)
    vertices[i].normal = Normalize(vertices[i].normal);
}

void GenerateSubdiviedCubeMeshData(MeshData *data, U32 cellsPerEdge)
{
	assert(data->vertexCount > 0);
	assert(data->indexCount > 0);
	Vertex3D *vertices = data->vertices;
	U32 *indices = data->indices;
	U32 vertexIndex = 0, indexIndex = 0;
	U32 verticesPerFace = (cellsPerEdge + 1) * (cellsPerEdge + 1);
	U32 indicesPerFace = (cellsPerEdge * cellsPerEdge) * 6;

	//Front
	for (U32 y = 0; y < cellsPerEdge + 1; y++) {
		for (U32 x = 0; x < cellsPerEdge + 1; x++) {
			Vertex3D &vertex = vertices[vertexIndex];
			vertex.position.x = -0.5f + (float)x / (float)cellsPerEdge;
			vertex.position.y = -0.5f + (float)y / (float)cellsPerEdge;
			vertex.position.z = 0.5f;
			vertex.normal = V3(0.0f, 0.0f, 1.0f);
			vertexIndex++;
		}
	}

	SetPlaneIndicesClockwise(cellsPerEdge, indices + (indicesPerFace * 0), 0 * verticesPerFace);

	//Back
	for (U32 y = 0; y < cellsPerEdge + 1; y++) {
		for (U32 x = 0; x < cellsPerEdge + 1; x++) {
			Vertex3D &vertex = vertices[vertexIndex];
			vertex.position.x = -0.5f + (float)x / (float)cellsPerEdge;
			vertex.position.y = -0.5f + (float)y / (float)cellsPerEdge;
			vertex.position.z = -0.5f;
			vertex.normal = V3(0.0f, 0.0f, -1.0f);
			vertexIndex++;
		}
	}
	SetPlaneIndicesCounterClockwise(cellsPerEdge, indices + (indicesPerFace * 1), 1 * verticesPerFace);

	//Left
	for (U32 y = 0; y < cellsPerEdge + 1; y++) {
		for (U32 x = 0; x < cellsPerEdge + 1; x++) {
			Vertex3D &vertex = vertices[vertexIndex];
			vertex.position.x = -0.5f;
			vertex.position.y = -0.5f + (float)y / (float)cellsPerEdge;
			vertex.position.z = -0.5f + (float)x / (float)cellsPerEdge;
			vertex.normal = V3(-1.0f, 0.0f, 0.0f);
			vertexIndex++;
		}
	}
	SetPlaneIndicesCounterClockwise(cellsPerEdge, indices + (indicesPerFace * 2), 2 * verticesPerFace);

	//Right
	for (U32 y = 0; y < cellsPerEdge + 1; y++) {
		for (U32 x = 0; x < cellsPerEdge + 1; x++) {
			Vertex3D &vertex = vertices[vertexIndex];
			vertex.position.x = 0.5f;
			vertex.position.y = -0.5f + (float)y / (float)cellsPerEdge;
			vertex.position.z = -0.5f + (float)x / (float)cellsPerEdge;
			vertex.normal = V3(1.0f, 0.0f, 0.0f);
			vertexIndex++;
		}
	}
	SetPlaneIndicesClockwise(cellsPerEdge, indices + (indicesPerFace * 3), 3 * verticesPerFace);

	//Top
	for (U32 y = 0; y < cellsPerEdge + 1; y++) {
		for (U32 x = 0; x < cellsPerEdge + 1; x++) {
			Vertex3D &vertex = vertices[vertexIndex];
			vertex.position.x = -0.5f + (float)y / (float)cellsPerEdge;
			vertex.position.y = 0.5f;
			vertex.position.z = -0.5f + (float)x / (float)cellsPerEdge;
			vertex.normal = V3(0.0f, 1.0f, 0.0f);
			vertexIndex++;
		}
	}
	SetPlaneIndicesClockwise(cellsPerEdge, indices + (indicesPerFace * 4), 4 * verticesPerFace);

	//Bottom
	for (U32 y = 0; y < cellsPerEdge + 1; y++) {
		for (U32 x = 0; x < cellsPerEdge + 1; x++) {
			Vertex3D &vertex = vertices[vertexIndex];
			vertex.position.x = -0.5f + (float)y / (float)cellsPerEdge;
			vertex.position.y = -0.5f;
			vertex.position.z = -0.5f + (float)x / (float)cellsPerEdge;
			vertex.normal = V3(0.0f, -1.0f, 0.0f);
			vertexIndex++;
		}
	}
	SetPlaneIndicesCounterClockwise(cellsPerEdge, indices + (indicesPerFace * 5), 5 * verticesPerFace);
}
