#include "ComponentPool.h"
#include "platypus/Common.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    ComponentPool::ComponentPool(
        ComponentType componentType,
        size_t componentSize,
        size_t componentCapacity,
        bool allowResize
    ) :
        MemoryPool(componentSize * componentCapacity),
        _componentType(componentType),
        _componentSize(componentSize),
        _componentCapacity(componentCapacity),
        _allowResize(allowResize)
    {}

    ComponentPool::ComponentPool(const ComponentPool& other) :
        _componentType(other._componentType),
        _componentSize(other._componentSize),
        _componentCapacity(other._componentCapacity),
        _componentCount(other._componentCount),
        _allowResize(other._allowResize)
    {
    }

    ComponentPool::~ComponentPool()
    {
    }

    ComponentPool::iterator ComponentPool::begin()
    {
        return { _pStorage, _componentSize, 0 };
    }

    ComponentPool::iterator ComponentPool::end()
    {
        return { nullptr, _componentSize, _occupiedSize };
    }

    void* ComponentPool::first()
    {
        if (_componentCount == 0)
            return nullptr;

        size_t offset = 0;
        // NOTE: Inefficient search, but atm shouldn't happen too often if user not dumb
        // TODO: User will be dumb -> Optimize this
        if (!_freeOffsets.empty())
        {
            for (size_t i = 0; i < _totalSize; i += _componentSize)
            {
                bool wasFreed = false;
                for (size_t j = 0; j < _freeOffsets.size(); ++j)
                {
                    if (i == _freeOffsets[j])
                        offset += _componentSize;
                }
            }
        }
        return (void*)(((PE_byte*)_pStorage) + offset * _componentSize);
    }

    void* ComponentPool::allocComponent(entityID_t entityID)
    {
        if (_occupiedSize + _componentSize > _totalSize)
        {
            if (!_allowResize)
            {
                Debug::log(
                    "@ComponentPool::allocComponent "
                    "Pool with resizing disabled was already full!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return nullptr;
            }
            else
            {
                addSpace(_totalSize + _componentSize);
                ++_componentCapacity;

                #ifdef PLATYPUS_DEBUG
                    Debug::log(
                        "@ComponentPool::" + component_type_to_string(_componentType) + "::allocComponent\n"
                        "   Increased pool size to: " + std::to_string(_totalSize) + "\n"
                        "   Component count: " + std::to_string(_componentCount)
                    );
                #endif
            }
        }
        void* ptr = nullptr;
        size_t allocationOffset = _componentCount;
        if (_freeOffsets.empty())
        {
            ptr = alloc(_componentSize);
        }
        else
        {
            allocationOffset = _freeOffsets.back();
            ptr = alloc(_componentSize * allocationOffset, _componentSize);
            _freeOffsets.pop_back();
        }
        if (!ptr)
        {
            Debug::log(
                "@ComponentPool::allocComponent "
                "Failed to allocate from memory pool",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        _componentCount++;
        _entityOffsetMapping[entityID] = allocationOffset;
        return ptr;
    }

    void ComponentPool::destroyComponent(entityID_t entityID)
    {
        if (_entityOffsetMapping.find(entityID) != _entityOffsetMapping.end())
        {
            size_t offset = _entityOffsetMapping[entityID];
            clearStorage(_componentSize * offset, _componentSize);
            _freeOffsets.push_back(offset);
            _entityOffsetMapping.erase(entityID);
            --_componentCount;
        }
        else
        {
            Debug::log(
                "@ComponentPool::destroyComponent "
                "Invalid entityID: " + std::to_string(entityID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void* ComponentPool::operator[](entityID_t entityID)
    {
        #ifdef PLATYPUS_DEBUG
        if (_entityOffsetMapping.find(entityID) == _entityOffsetMapping.end())
        {
            Debug::log(
                "@ComponentPool::operator[] (1)"
                "Failed to find entity: " + std::to_string(entityID) + " from pool",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        #endif
        size_t offset = _entityOffsetMapping[entityID];
        return (void*)(((PE_byte*)_pStorage) + offset * _componentSize);
    }

    const void* ComponentPool::operator[](entityID_t entityID) const
    {
        // Maybe use const ref const_iterator here? Or is it too dangerous?
        std::unordered_map<entityID_t, size_t>::const_iterator it = _entityOffsetMapping.find(entityID);
        if (it == _entityOffsetMapping.end())
        {
            Debug::log(
                "@ComponentPool::operator[] (2)"
                "Failed to find entity: " + std::to_string(entityID) + " from pool",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        size_t offset = it->second;
        return (const void*)(((PE_byte*)_pStorage) + offset * _componentSize);
    }
}
