#pragma once

#include "Asset.h"
#include "platypus/graphics/Buffers.h"


namespace platypus
{
    class Mesh : public Asset
    {
    private:
        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;

    public:
        // NOTE: Ownership of vertex and index buffer gets transferred to this Mesh
        Mesh(Buffer* pVertexBuffer, Buffer* pIndexBuffer);
        ~Mesh();

        inline const Buffer* getVertexBuffer() const { return _pVertexBuffer; }
        inline const Buffer* getIndexBuffer() const { return _pIndexBuffer; }
    };
}
