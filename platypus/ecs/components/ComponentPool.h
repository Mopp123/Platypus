#pragma once

#include "platypus/core/Memory.h"
#include "platypus/ecs/Entity.h"
#include "Component.h"
#include <vector>
#include <unordered_map>


namespace platypus
{
    class ComponentPool : public MemoryPool
    {
    private:
        ComponentType _componentType;
        size_t _componentSize = 0; // Size of a single component
        size_t _componentCapacity = 0; // How many components can be allocated
        size_t _componentCount = 0;
        bool _allowResize = false;

        std::vector<size_t> _freeOffsets;
        // Actual offset of entity's component in storage is determined by this.
        // NOTE: VERY IMPORTANT! Here when we talk about offsets we mean
        // component index and not the actual byte offset in _pStorage
        // unlike in MemoryPool!
        std::unordered_map<entityID_t, size_t> _entityOffsetMapping;

    public:
        struct iterator
        {
            void* ptr;
            size_t componentSize = 0;
            size_t position = 0;

            bool operator!=(const iterator& other)
            {
                return position != other.position;
            }

            iterator& operator++()
            {
                ptr = ((uint8_t*)ptr) + componentSize;
                position += componentSize;
                return *this;
            }
        };

        ComponentPool() {}
        ComponentPool(
            ComponentType componentType,
            size_t componentSize,
            size_t componentCapacity,
            bool allowResize
        );
        ComponentPool(const ComponentPool& other);
        ~ComponentPool();

        // NOTE: Iterator to beginning of memory, NOT THE ACTUAL EXISTING COMPONENT!
        // NOTE: Not even sure where this is used...?
        iterator begin();
        iterator end();

        // Returns ptr to first existing component.
        // Takes possible destroyed components from middle, beginning, etc into account.
        void* first();

        // If allowResize and not enough space, this also adds space to the _pStorage
        void* allocComponent(entityID_t entityID);
        void destroyComponent(entityID_t entityID);

        void* operator[](entityID_t entityID);
        const void* operator[](entityID_t entityID) const;

        inline size_t getComponentCount() const { return _componentCount; }
    };
}
