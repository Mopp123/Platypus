#include "Mesh.h"
#include "platypus/core/Debug.h"
#include "platypus/Common.h"
#include <cmath>


namespace platypus
{
    std::string mesh_type_to_string(MeshType type)
    {
        switch (type)
        {
            case MeshType::MESH_TYPE_STATIC: return "MESH_TYPE_STATIC";
            case MeshType::MESH_TYPE_STATIC_INSTANCED: return "MESH_TYPE_STATIC_INSTANCED";
            case MeshType::MESH_TYPE_SKINNED: return "MESH_TYPE_SKINNED";
            default: return "<Invalid type>";
        }
    }


    Mesh::Mesh(
        MeshType type,
        VertexBufferLayout vertexBufferLayout,
        Buffer* pVertexBuffer,
        Buffer* pIndexBuffer,
        const Matrix4f& transformationMatrix,
        Pose bindPose
    ) :
        Asset(AssetType::ASSET_TYPE_MESH),
        _type(type),
        _vertexBufferLayout(vertexBufferLayout),
        _pVertexBuffer(pVertexBuffer),
        _pIndexBuffer(pIndexBuffer),
        _transformationMatrix(transformationMatrix)
    {
        _bindPose = bindPose;
    }

    Mesh::~Mesh()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }

    Mesh* Mesh::generate_terrain(
        float tileSize,
        const std::vector<float>& heightmapData,
        bool dynamic,
        bool generateTangents
    )
    {
        const float minTileSize = 0.05f;
        if (tileSize < minTileSize)
        {
            Debug::log(
                "@Mesh::generate_terrain "
                "Invalid tileSize(" + std::to_string(tileSize) + ") "
                "Minimum tile size must be greater than " + std::to_string(minTileSize),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (heightmapData.size() < 4)
        {
            Debug::log(
                "@Mesh::generate_terrain "
                "Heightmap's size must be greater or equal to 2x2",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

		size_t verticesPerRow = sqrt(heightmapData.size());
        size_t tilesPerRow = verticesPerRow - 1;

        const size_t vertexCount = verticesPerRow * verticesPerRow;
        const size_t stride = sizeof(Vector3f) * 2 + sizeof(Vector2f) + (generateTangents ? sizeof(Vector4f) : 0);
        const size_t dataSize = verticesPerRow * verticesPerRow * stride;
        std::vector<PE_byte> vertexData(dataSize);
        size_t dataOffset = 0;
        for (int z = 0; z < (int)verticesPerRow; ++z)
        {
            for (int x = 0; x < (int)verticesPerRow; ++x)
            {
                float height = heightmapData[x + z * verticesPerRow];
                Vector3f position = { x * tileSize, height, z * tileSize };

                /*
                    const float tileSize = instanceData.meshProperties.x;
                    const float verticesPerRow = instanceData.meshProperties.y;
                    const float tilesPerRow = verticesPerRow - 1;
                    var_texCoord = vec2(position.x / tileSize / tilesPerRow, position.z / tileSize / tilesPerRow);
                */
                Vector2f uv(
                    position.x / (float)tileSize / (float)tilesPerRow,
                    position.z / (float)tileSize / (float)tilesPerRow
                );

				float left = 0;
				float right = 0;
				float down = 0;
				float up = 0;

				if (x - 1 >= 0)
					left = heightmapData[(x - 1) + z * verticesPerRow];

				if (x + 1 < (int)verticesPerRow)
					right = heightmapData[(x + 1) + z * verticesPerRow];

				if (z + 1 < (int)verticesPerRow)
					up = heightmapData[x + (z + 1) * verticesPerRow];

				if (z - 1 >= 0)
					down = heightmapData[x + (z - 1) * verticesPerRow];

				Vector3f normal((left - right), 1.0f, (down - up)); // this is pretty dumb...

                PE_byte* pTarget = vertexData.data();
                memcpy((void*)(pTarget + dataOffset), &position, sizeof(Vector3f));
                memcpy((void*)(pTarget + dataOffset + sizeof(Vector3f)), &normal, sizeof(Vector3f));
                memcpy((void*)(pTarget + dataOffset + sizeof(Vector3f) * 2), &uv, sizeof(Vector2f));

                if (generateTangents)
                {
                    Vector4f tangent(1, 0, 0, 0);
                    if (x + 1 < verticesPerRow)
                    {
                        float rightHeight = heightmapData[(x + 1) + z * verticesPerRow];
                        Vector4f rightVertexPosition = { (x + 1) * tileSize, rightHeight, z * tileSize, 0 };
                        if (rightVertexPosition.length() > 0.0f)
                        {
                            Vector4f posVec4(position.x, position.y, position.z, 0.0f);
                            tangent = rightVertexPosition - posVec4;
                            tangent = tangent.normalize();
                        }
                    }
                    memcpy((void*)(pTarget + dataOffset + sizeof(Vector3f) * 2 + sizeof(Vector2f)), &tangent, sizeof(Vector4f));
                }

                dataOffset += stride;
            }
        }

        std::vector<uint32_t> indices;
		for (uint32_t x = 0; x < verticesPerRow; x++)
		{
			for (uint32_t z = 0; z < verticesPerRow; z++)
			{
				if (x >= verticesPerRow - 1 || z >= verticesPerRow - 1)
					continue;

				uint32_t topLeft = x + z * verticesPerRow;
				uint32_t bottomLeft = x + (z + 1) * verticesPerRow;

				uint32_t topRight = (x + 1) + z * verticesPerRow;
				uint32_t bottomRight = (x + 1) + (z + 1) * verticesPerRow;

                indices.insert(
                    indices.end(),
                    {
                        topLeft, bottomLeft, bottomRight,
                        bottomRight, topRight, topLeft
                    }
                );

                // Below original order...
                /*
				indices.push_back(bottomLeft);
				indices.push_back(topLeft);
				indices.push_back(topRight);

				indices.push_back(topRight);
				indices.push_back(bottomRight);
				indices.push_back(bottomLeft);
                */
			}
		}

        uint32_t bufferUsageFlags = BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT;
        BufferUpdateFrequency updateFrequency = BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC;
        bool storeHostSide = false;
        if (dynamic)
        {
            // NOTE: Too inefficient, not having mem device local in case of dynamic terrain?
            bufferUsageFlags = BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT;
            // NOTE: Not sure, should use DYNAMIC or STREAM in this case...
            updateFrequency = BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC;
        }

        Buffer* pVertexBuffer = new Buffer(
            vertexData.data(),
            stride,
            vertexCount,
            bufferUsageFlags,
            updateFrequency,
            storeHostSide
        );
        Buffer* pIndexBuffer = new Buffer(
            indices.data(),
            sizeof(uint32_t),
            indices.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false // store host side
        );

        Mesh* pMesh = new Mesh(
            MeshType::MESH_TYPE_STATIC,
            generateTangents ? VertexBufferLayout::get_common_static_tangent_layout() : VertexBufferLayout::get_common_static_layout(),
            pVertexBuffer,
            pIndexBuffer,
            Matrix4f(1.0f),
            { }
        );
        return pMesh;
    }
}
