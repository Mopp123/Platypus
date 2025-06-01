#include "Mesh.h"


namespace platypus
{
    Mesh::Mesh(
        VertexBufferLayout vertexBufferLayout,
        Buffer* pVertexBuffer,
        Buffer* pIndexBuffer,
        const Matrix4f& transformationMatrix,
        Pose bindPose
    ) :
        Asset(AssetType::ASSET_TYPE_MESH),
        _vertexBufferLayout(vertexBufferLayout),
        _pVertexBuffer(pVertexBuffer),
        _pIndexBuffer(pIndexBuffer),
        _transformationMatrix(transformationMatrix),
        _bindPose(bindPose)
    {}

    Mesh::~Mesh()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }
}
