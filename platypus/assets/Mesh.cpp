#include "Mesh.h"


namespace platypus
{
    Mesh::Mesh(
        Buffer* pVertexBuffer,
        Buffer* pIndexBuffer,
        const Matrix4f& transformationMatrix
    ) :
        Asset(AssetType::ASSET_TYPE_MESH),
        _pVertexBuffer(pVertexBuffer),
        _pIndexBuffer(pIndexBuffer),
        _transformationMatrix(transformationMatrix)
    {}

    Mesh::~Mesh()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }
}
