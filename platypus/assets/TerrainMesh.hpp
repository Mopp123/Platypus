#pragma once

#include "Asset.h"
#include "platypus/graphics/Buffers.h"
#include "Image.h"
#include "platypus/utils/Maths.h"


namespace platypus
{
    class TerrainMesh : public Asset
    {
    private:
        VertexBufferLayout _vertexBufferLayout;
        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;

        float _tileSize = 1.0f;
        uint32_t _verticesPerRow = 2;

    public:
        TerrainMesh(
            float tileSize,
            const std::vector<float>& heightmapData,
            bool dynamic,
            bool generateTangents
        );

        ~TerrainMesh();

        inline const VertexBufferLayout& getVertexBufferLayout() const { return _vertexBufferLayout; }
        inline const Buffer* getVertexBuffer() const { return _pVertexBuffer; }
        inline Buffer* getVertexBuffer() { return _pVertexBuffer; }
        inline const Buffer* getIndexBuffer() const { return _pIndexBuffer; }

        inline float getTileSize() const { return _tileSize; }
        inline uint32_t getVerticesPerRow() const { return _verticesPerRow; }
    };
}
