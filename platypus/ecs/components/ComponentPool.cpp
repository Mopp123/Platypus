#include "ComponentPool.hpp"
#include "platypus/core/Debug.h"

#include "Camera.h"
#include "Lights.h"
#include "Renderable.h"
#include "SkeletalAnimation.h"
#include "Transform.h"

#include <cstring>


namespace platypus
{
    template class ComponentPool<Camera>;
    template class ComponentPool<Light>;
    template class ComponentPool<Renderable3D>;
    template class ComponentPool<GUIRenderable>;
    template class ComponentPool<Transform>;
    template class ComponentPool<GUITransform>;
    template class ComponentPool<SkeletalAnimation>;
    template class ComponentPool<SkeletonJoint>;
    template class ComponentPool<Parent>;
    template class ComponentPool<Children>;

    template <typename T>
    ComponentPool<T>::ComponentPool(
        size_t componentSize,
        size_t maxLength,
        bool allowResize
    ) :
        MemoryPool(componentSize, maxLength),
        _allowResize(allowResize)
    {}

    template <typename T>
    ComponentPool<T>::ComponentPool(const ComponentPool& other) :
        MemoryPool(other), // NOTE: Not sure is this fine?
        _allowResize(other._allowResize),
        _entityIndexMapping(other._entityIndexMapping)
    {
    }

    template <typename T>
    ComponentPool<T>::~ComponentPool()
    {
    }

    template <typename T>
    int32_t ComponentPool<T>::userDataToIndex(void* pUserData) const
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

    template <typename T>
    void ComponentPool<T>::constructElement(size_t index, void* pData, void* pUserData)
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

    template <typename T>
    void ComponentPool<T>::destroyElement(size_t index, void* pData, void* pUserData)
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

    template <typename T>
    void ComponentPool<T>::onClearFullStorage()
    {
        _entityIndexMapping.clear();
    }

    template <typename T>
    void ComponentPool<T>::onFreeStorage()
    {
        _entityIndexMapping.clear();
    }

    template <typename T>
    void* ComponentPool<T>::onStorageFull()
    {
        size_t newElementCount = _totalLength + 1;
        #ifdef PLATYPUS_DEBUG
        Debug::log(
            "Component pool storage was full(elements = " + std::to_string(_occupiedCount) + " "
            "total size = " + std::to_string(getTotalSize()) + ") "
            "-> resizing to " + std::to_string(newElementCount),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_WARNING
        );
        #endif

        addSpace(newElementCount);
        return nullptr;
    }
}
