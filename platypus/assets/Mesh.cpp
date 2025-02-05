#include "Mesh.h"


namespace platypus
{
    Mesh::Mesh(Buffer* pVertexBuffer, Buffer* pIndexBuffer) :
        Asset(AssetType::ASSET_TYPE_MESH),
        _pVertexBuffer(pVertexBuffer),
        _pIndexBuffer(pIndexBuffer)
    {}

    Mesh::~Mesh()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }
}
