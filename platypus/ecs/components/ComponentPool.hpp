#pragma once

#include "platypus/core/Memory.hpp"
#include "platypus/core/Debug.hpp"

#include "platypus/ecs/Entity.hpp"

#include <cstring>
#include <unordered_map>


namespace platypus
{
    template <typename T>
    class ComponentPool : public MemoryPool
    {
    private:
        // Actual index for entity in the pool
        std::unordered_map<entityID_t, size_t> _entityIndexMapping;

    public:
        ComponentPool() {}
        ComponentPool(
            size_t componentSize,
            size_t maxLength,
            bool allowResize
        ) :
            MemoryPool(componentSize, maxLength, allowResize)
        {}

        ComponentPool(const ComponentPool& other) :
            MemoryPool(other), // NOTE: Not sure is this fine?
            _entityIndexMapping(other._entityIndexMapping)
        {}

        ~ComponentPool() {}

        virtual int32_t userDataToIndex(void* pUserData) const
        {
            if (pUserData)
            {
                // NOTE: DANGER!?
                entityID_t entity;
                memcpy(reinterpret_cast<void*>(&entity), pUserData, sizeof(entityID_t));
                std::unordered_map<entityID_t, size_t>::const_iterator it = _entityIndexMapping.find(entity);
                if (it != _entityIndexMapping.end())
                    return it->second;
            }
            return -1;
        }

        virtual void constructElement(size_t index, void* pData, void* pUserData)
        {
            // NOTE: DANGER!?
            entityID_t entity;
            memcpy(reinterpret_cast<void*>(&entity), pUserData, sizeof(entityID_t));

            #ifdef PLATYPUS_DEBUG
            if (_entityIndexMapping.find(entity) != _entityIndexMapping.end())
            {
                Debug::log(
                    "Index(" + std::to_string(index) + ") already exists for entity " + std::to_string(entity),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            #endif
            T* pComponent = new (pData) T;
            _entityIndexMapping[entity] = index;
        }

        virtual void destroyElement(size_t index, void* pData, void* pUserData)
        {
            // NOTE: DANGER!?
            if (pUserData)
            {
                entityID_t entity = NULL_ENTITY_ID;
                memcpy(reinterpret_cast<void*>(&entity), pUserData, sizeof(entityID_t));
                #ifdef PLATYPUS_DEBUG
                if (_entityIndexMapping.find(entity) == _entityIndexMapping.end())
                {
                    Debug::log(
                        "No index exists for entity " + std::to_string(entity),
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }
                #endif
                _entityIndexMapping.erase(entity);
            }

            T* pComponent = reinterpret_cast<T*>(pData);
            pComponent->~T();
        }

        virtual void onClearFullStorage()
        {
            _entityIndexMapping.clear();
        }

        virtual void onFreeStorage()
        {
            _entityIndexMapping.clear();
        }
    };
}
