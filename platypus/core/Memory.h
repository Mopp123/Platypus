#pragma once

#include <cstddef>


namespace platypus
{
    class MemoryPool
    {
    protected:
        size_t _totalSize = 0;
        size_t _occupiedSize = 0;
        void* _pStorage = nullptr;

    public:
        MemoryPool() {}
        MemoryPool(size_t size);
        MemoryPool(const MemoryPool& other);
        virtual ~MemoryPool();

        // Allocates chunk of size "size" in pool and returns ptr to that
        void* alloc(size_t size);
        void* alloc(size_t offset, size_t size);

        // Clears full storage but doesn't resize the actual storage
        // (sets all data and _occupeidSize to 0)
        void clearStorage();
        // Clears storage of specified area
        void clearStorage(size_t offset, size_t size);

        // Calls free for _pStorage and sets it to nullptr
        void freeStorage();
        void addSpace(size_t newSize);

        inline size_t getSize() const { return _occupiedSize; }
        inline size_t getCapacity() const { return _totalSize; }

        inline void* accessStorage() { return _pStorage; }
    };
}
