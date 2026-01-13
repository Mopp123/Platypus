#pragma once

#include "Asset.hpp"
#include "platypus/graphics/Buffers.h"
#include "platypus/utils/Maths.h"
#include "platypus/utils/AnimationDataUtils.h"


namespace platypus
{
    enum MeshType
    {
        MESH_TYPE_NONE = 0x0,
        MESH_TYPE_STATIC = 0x1,
        MESH_TYPE_STATIC_INSTANCED = 0x1 << 1,
        MESH_TYPE_SKINNED = 0x1 << 2,
    };
    std::string mesh_type_to_string(MeshType type);

    class Mesh : public Asset
    {
    private:
        MeshType _type;
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
            MeshType type,
            VertexBufferLayout vertexBufferLayout,
            Buffer* pVertexBuffer,
            Buffer* pIndexBuffer,
            const Matrix4f& transformationMatrix,
            Pose bindPose
        );

        ~Mesh();

        static Mesh* generate_terrain(
            float tileSize,
            const std::vector<float>& heightmapData,
            bool dynamic,
            bool generateTangents
        );

        inline MeshType getType() const { return _type; }
        inline const VertexBufferLayout& getVertexBufferLayout() const { return _vertexBufferLayout; }
        inline const Buffer* getVertexBuffer() const { return _pVertexBuffer; }
        inline Buffer* getVertexBuffer() { return _pVertexBuffer; }
        inline const Buffer* getIndexBuffer() const { return _pIndexBuffer; }
        inline const Matrix4f getTransformationMatrix() const { return _transformationMatrix; }
        inline bool hasBindPose() const { return !_bindPose.joints.empty(); }
        inline size_t getJointCount() const { return _bindPose.joints.size(); }
        inline const Pose& getBindPose() const { return _bindPose; }
        inline const Pose* getBindPosePtr() const { return &_bindPose; }
    };
}
