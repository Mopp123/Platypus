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

    class AssetManager;
    class Mesh : public Asset
    {
    private:
        uint32_t _propertyFlags = 0;
        VertexBufferLayout _vertexBufferLayout;
        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;
        bool _storeHostsideBuffersOnDeserialization = false;

        // Transformation from the file this was loaded from, if there was any.
        // Not sure yet how I want to deal with this.
        Matrix4f _transformationMatrix = Matrix4f(1.0f);

        // NOTE:
        // *This was previously Pose _bindPose!
        // *Then this was Skeleton* _pSkeleton...
        UUID_t _skeletonID = NULL_UUID;

    public:
        // NOTE: Ownership of vertex and index buffer gets transferred to this Mesh
        Mesh(
            size_t uuidPool,
            uint32_t propertyFlags,
            VertexBufferLayout vertexBufferLayout,
            Buffer* pVertexBuffer,
            Buffer* pIndexBuffer,
            const Matrix4f& transformationMatrix,
            UUID_t skeletonID,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        Mesh(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );
        ~Mesh();

        bool hasTangents() const;

        static Mesh* generate_terrain(
            size_t uuidPool,
            float tileSize,
            const std::vector<float>& heightmapData,
            bool dynamic,
            bool generateTangents
        );

        virtual void serialize(
            std::vector<char>& targetBuffer
        ) const override;

        virtual size_t getSerializedSize() const override;

        Skeleton* getSkeleton() const;

        inline uint32_t getPropertyFlags() const { return _propertyFlags; }
        inline const VertexBufferLayout& getVertexBufferLayout() const { return _vertexBufferLayout; }
        inline const Buffer* getVertexBuffer() const { return _pVertexBuffer; }
        inline Buffer* getVertexBuffer() { return _pVertexBuffer; }
        inline const Buffer* getIndexBuffer() const { return _pIndexBuffer; }
        inline bool isStoringHostsideBuffersOnDeserialization() const { return _storeHostsideBuffersOnDeserialization; }
        inline void storeHostsideBuffersOnDeserialization(bool arg) { _storeHostsideBuffersOnDeserialization = arg; }
        inline const Matrix4f getTransformationMatrix() const { return _transformationMatrix; }
        inline UUID_t getSkeletonID() const { return _skeletonID; }
    };
}
