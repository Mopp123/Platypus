#pragma once

#include "platypus/core/Memory.h"
#include "platypus/ecs/Entity.h"
#include <unordered_map>


namespace platypus
{
    template <typename T>
    class ComponentPool : public MemoryPool
    {
    private:
        bool _allowResize = false;

        // Actual index for entity in the pool
        std::unordered_map<entityID_t, size_t> _entityIndexMapping;

    public:
        ComponentPool() {}
        ComponentPool(
            size_t componentSize,
            size_t maxLength,
            bool allowResize
        );
        ComponentPool(const ComponentPool& other);
        ~ComponentPool();

        virtual int32_t userDataToIndex(void* pUserData) const;
        virtual void constructElement(size_t index, void* pData, void* pUserData);
        virtual void destroyElement(size_t index, void* pData, void* pUserData);
        virtual void onClearFullStorage();
        virtual void onFreeStorage();

    NOTE: RATHER MAKE RESIZING HAPPEN VIA MemoryPool, NOT HERE!
        virtual void* onStorageFull() override;
    };
}
