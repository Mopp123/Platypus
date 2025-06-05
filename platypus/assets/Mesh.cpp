#include "Mesh.h"


namespace platypus
{
    Mesh::Mesh(
        VertexBufferLayout vertexBufferLayout,
        Buffer* pVertexBuffer,
        Buffer* pIndexBuffer,
        const Matrix4f& transformationMatrix
    ) :
        Asset(AssetType::ASSET_TYPE_MESH),
        _vertexBufferLayout(vertexBufferLayout),
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
