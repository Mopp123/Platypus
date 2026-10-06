#pragma once
#include "platypus/utils/UUID.hpp"
#include "platypus/assets/Asset.hpp"
#include "platypus/core/Memory.hpp"
#include <vector>
#include <cstdint>

#define NULL_ENTITY_ID -1
typedef int64_t entityID_t;


namespace platypus
{
    class Scene;

    constexpr size_t serialized_entity_base_size =
        sizeof(UUID_t) +
        sizeof(uint64_t) + // component mask
        sizeof(uint8_t) + // active
        sizeof(uint32_t); // name size

    constexpr size_t serialized_entities_header_size = sizeof(uint32_t);


    enum class EntityErrorType : uint32_t
    {
        NO_ERROR = 0,
        COMPONENT_RENDERABLE3D_MESH_UNAVAILABLE,
        COMPONENT_RENDERABLE3D_MATERIAL_UNAVAILABLE,
        COMPONENT_RENDERABLE3D_INCOMPATIBLE_MESH_MATERIAL
    };

    std::string entity_error_type_to_string(EntityErrorType error);

    struct EntityError
    {
        EntityErrorType type;
        // pair's first = ComponentType!
        std::set<std::pair<uint32_t, void*>> targetComponents;
        std::set<Asset*> targetAssets;
    };

    void handle_mesh_unavailable_error(Scene* pScene, EntityError error);
    void handle_material_unavailable_error(Scene* pScene, EntityError error);
    void handle_incompatible_mesh_material_error(Scene* pScene, EntityError error);


    struct Entity
    {
        entityID_t id = NULL_ENTITY_ID;
        UUID_t uuid = NULL_UUID;

        uint64_t componentMask = 0;
        uint8_t active = 1;

        Entity();
        Entity(const Entity& other);
        void clear(uint32_t UUIDPoolID);
    };


    // NOTE: Doesn't work if the mask value's size changes!
    size_t get_component_count(uint64_t componentMask);

    size_t get_serialized_entity_size(const Scene* pScene, const Entity& entity);
    // NOTE:
    // *entityID_t not included in serialized format, ONLY THE UUID!
    std::vector<char> serialize_entity(
        const Scene* pScene,
        const Entity& entity
    );
    void deserialize_entity(
        Scene* pScene,
        Entity& outEntity,
        const void* pData
    );


    struct Children;
    class EntityHierarchyManager
    {
    private:
        DynamicElementSizeMemoryPool _memoryPool;
        Scene* _pScene = nullptr;
        const size_t _elementSize = sizeof(entityID_t);

    public:
        EntityHierarchyManager(Scene* pScene);

        void addChild(Children* pChildrenComponent, entityID_t childEntityID);
        void removeChild(Children* pChildrenComponent, entityID_t childEntityID);

        const entityID_t* getChildEntityIDs(const Children * const pChildrenComponent) const;

    private:
        inline size_t getComponentStorageSize(size_t count) const { return count * _elementSize; }

        static void free_range_func(size_t offset, size_t size, void* pUserData);
        static bool validate_free_range_func(size_t offset, size_t size, void* pUserData);
    };
}
