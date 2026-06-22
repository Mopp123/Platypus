#pragma once
#include "platypus/utils/UUID.hpp"
#include "platypus/assets/Asset.hpp"
#include <vector>
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
}
