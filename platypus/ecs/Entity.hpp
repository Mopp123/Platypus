#pragma once
#include "platypus/utils/UUID.hpp"
#include "platypus/assets/Asset.hpp"
#include <vector>
#include <map>
#include <cstdint>

#define NULL_ENTITY_ID -1
typedef int64_t entityID_t;


namespace platypus
{
    class Scene;

    constexpr size_t serialized_entity_name_size = 64;
    constexpr size_t serialized_entity_size =
        sizeof(UUID_t) +
        sizeof(uint64_t) +
        sizeof(uint8_t) +
        serialized_entity_name_size;

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

    // NOTE:
    // *entityID_t not included in serialized format, ONLY THE UUID!
    // *Should we rather return the UUID instead of the entityID_t here?
    std::vector<char> serialize_entity(const Entity& entity, const std::string& entityName);
    void deserialize_entity(
        Scene* pScene,
        Entity& outEntity,
        size_t dataSize,
        const void* pData
    );


    class Scene;
    struct Children;
    class EntityHierarchyManager
    {
    private:
        Scene* _pScene = nullptr;
        // This contains every Children component's used child entityID_ts
        std::vector<entityID_t> _childrenContainer;
        // key = offset, value = count
        std::map<size_t, size_t> _freeRanges;

    public:
        EntityHierarchyManager(Scene* pScene);

        // Returns the offset where child entities begin in _childrenContainer or
        // -1 if fails to occupy
        int32_t occupyRange(const std::vector<entityID_t>& childEntities);

        void freeRange(int32_t offset, size_t count);

        // Changes the previously used offset and returns it
        int32_t addChild(
            const Children * const pChildren,
            entityID_t childEntityID
        );

        void removeChild(
            const Children * const pChildren,
            entityID_t childEntityID
        );

        const entityID_t* getChildEntities(const Children * const pChildren) const;

    private:
        int32_t findFreeRange(size_t requiredCount);
        bool validateFreeRange(size_t offset, size_t count) const;

        // Makes all children to be contiguous
        void packChildren(size_t beginOffset, size_t freeOffset, size_t count);
        // NOTE: this is too complicated, inefficient and dumb
        // TODO: Improve, optimize ..or something...
        void packFreeRanges();
    };
}
