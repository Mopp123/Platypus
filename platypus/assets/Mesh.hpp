#pragma once

#include "Asset.hpp"
#include "SkeletalAnimationData.hpp"
#include "platypus/graphics/Buffers.hpp"
#include "platypus/utils/Maths.hpp"
#include "platypus/utils/AnimationDataUtils.hpp"


namespace platypus
{

    enum class MeshPropertyFlagBits : uint32_t
    {
        NONE = 0,
        TYPE_STATIC = 0x1,
        TYPE_SKINNED = 0x1 << 1,

        HAS_TANGENTS = 0x1 << 2,
        INSTANCED = 0x1 << 3
    };
    MeshPropertyFlagBits get_mesh_type(uint32_t meshPropertyFlags);
    std::string mesh_type_to_string(MeshPropertyFlagBits type);

    class Mesh : public Asset
    {
    private:
        uint32_t _propertyFlags = 0;
        VertexBufferLayout _vertexBufferLayout;
        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;

        // Transformation from the file this was loaded from, if there was any.
        // Not sure yet how I want to deal with this.
        Matrix4f _transformationMatrix = Matrix4f(1.0f);

        Pose _bindPose;

        // NOTE: Not sure if "assets inside assets" should rather be IDs to those instead of ptrs!
        //  -> ptrs will become issue when serializing
        std::vector<SkeletalAnimationData*> _animations;

    public:
        // NOTE: Ownership of vertex and index buffer gets transferred to this Mesh
        Mesh(
            size_t uuidPool,
            uint32_t propertyFlags,
            VertexBufferLayout vertexBufferLayout,
            Buffer* pVertexBuffer,
            Buffer* pIndexBuffer,
            const Matrix4f& transformationMatrix,
            Pose bindPose,
            const std::vector<SkeletalAnimationData*>& animations,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        ~Mesh();

        // returns -1 if not found
        int32_t getAnimationIndex(const std::string& name) const;

        static Mesh* generate_terrain(
            size_t uuidPool,
            float tileSize,
            const std::vector<float>& heightmapData,
            bool dynamic,
            bool generateTangents
        );

        inline const std::vector<SkeletalAnimationData*>& getAnimations() const { return _animations; }

        inline uint32_t getPropertyFlags() const { return _propertyFlags; }
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
