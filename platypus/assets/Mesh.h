#pragma once

#include "Asset.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/utils/Maths.h"
#include "platypus/utils/SkeletalAnimationData.h"


namespace platypus
{
    class Mesh : public Asset
    {
    private:
        VertexBufferLayout _vertexBufferLayout;
        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;

        // Transformation from the file this was loaded from, if there was any.
        // Not sure yet how I want to deal with this.
        Matrix4f _transformationMatrix = Matrix4f(1.0f);

        Pose _bindPose;

    public:
        // NOTE: Ownership of vertex and index buffer gets transferred to this Mesh
        Mesh(
            VertexBufferLayout vertexBufferLayout,
            Buffer* pVertexBuffer,
            Buffer* pIndexBuffer,
            const Matrix4f& transformationMatrix,
            Pose bindPose = {{},{}}
        );

        ~Mesh();

        inline const VertexBufferLayout& getVertexBufferLayout() const { return _vertexBufferLayout; }
        inline const Buffer* getVertexBuffer() const { return _pVertexBuffer; }
        inline const Buffer* getIndexBuffer() const { return _pIndexBuffer; }
        inline const Matrix4f getTransformationMatrix() const { return _transformationMatrix; }
        inline const Pose& getBindPose() const { return _bindPose; }
        inline void setBindPose(const Pose& pose) { _bindPose = pose; }
    };
}
