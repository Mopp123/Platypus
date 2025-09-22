#include "TerrainMesh.hpp"
#include "platypus/Common.h"
#include "platypus/core/Debug.h"
#include "platypus/utils/Maths.h"
#include <cmath>


namespace platypus
{
    VertexBufferLayout TerrainMesh::s_vertexBufferLayout =
    {
        {
            { 0, ShaderDataType::Float3 },
            { 1, ShaderDataType::Float3 }
        },
        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
        0
    };

    TerrainMesh::TerrainMesh(
        float tileSize,
        const std::vector<float>& heightmapData,
        bool dynamic
    ) :
        Asset(AssetType::ASSET_TYPE_TERRAIN_MESH),
        _tileSize(tileSize)
    {
        const float minTileSize = 0.05f;
        if (tileSize < minTileSize)
        {
            Debug::log(
                "@TerrainMesh::TerrainMesh "
                "Invalid tileSize(" + std::to_string(tileSize) + ") "
                "Minimum tile size must be greater than " + std::to_string(minTileSize),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (heightmapData.size() < 4)
        {
            Debug::log(
                "@TerrainMesh::TerrainMesh "
                "Heightmap's size must be greater or equal to 2x2",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
		_verticesPerRow = sqrt(heightmapData.size());
        const size_t vertexCount = _verticesPerRow * _verticesPerRow;
        // TODO: Generate with heightmap
        const size_t stride = sizeof(Vector3f) * 2;
        const size_t dataSize = _verticesPerRow * _verticesPerRow * stride;
        std::vector<PE_byte> vertexData(dataSize);
        size_t dataOffset = 0;
        for (int z = 0; z < (int)_verticesPerRow; ++z)
        {
            for (int x = 0; x < (int)_verticesPerRow; ++x)
            {
                float height = heightmapData[x + z * _verticesPerRow];
                Vector3f position = { x * _tileSize, height, z * _tileSize };

				float left = 0;
				float right = 0;
				float down = 0;
				float up = 0;

				if (x - 1 >= 0)
					left = heightmapData[(x - 1) + z * _verticesPerRow];

				if (x + 1 < (int)_verticesPerRow)
					right = heightmapData[(x + 1) + z * _verticesPerRow];

				if (z + 1 < (int)_verticesPerRow)
					up = heightmapData[x + (z + 1) * _verticesPerRow];

				if (z - 1 >= 0)
					down = heightmapData[x + (z - 1) * _verticesPerRow];

				Vector3f normal((left - right), 1.0f, (down - up)); // this is pretty dumb...

                PE_byte* pTarget = vertexData.data();
                memcpy((void*)(pTarget + dataOffset), &position, sizeof(Vector3f));
                memcpy((void*)(pTarget + dataOffset + sizeof(Vector3f)), &normal, sizeof(Vector3f));
                dataOffset += stride;
            }
        }

        std::vector<uint32_t> indices;
		for (uint32_t x = 0; x < _verticesPerRow; x++)
		{
			for (uint32_t z = 0; z < _verticesPerRow; z++)
			{
				if (x >= _verticesPerRow - 1 || z >= _verticesPerRow - 1)
					continue;

				uint32_t topLeft = x + (z + 1) * _verticesPerRow;
				uint32_t bottomLeft = x + z * _verticesPerRow;

				uint32_t topRight = (x + 1) + (z + 1) * _verticesPerRow;
				uint32_t bottomRight = (x + 1) + z * _verticesPerRow;

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
            storeHostSide = true;
        }

        _pVertexBuffer = new Buffer(
            vertexData.data(),
            stride,
            vertexCount,
            bufferUsageFlags,
            updateFrequency,
            storeHostSide
        );
        _pIndexBuffer = new Buffer(
            indices.data(),
            sizeof(uint32_t),
            indices.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false // store host side
        );
    }

    TerrainMesh::~TerrainMesh()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }

    VertexBufferLayout TerrainMesh::get_vertex_buffer_layout()
    {
        return s_vertexBufferLayout;
    }
}
