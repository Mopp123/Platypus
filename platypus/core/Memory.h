#pragma once

#include <cstddef>
#include <cstdint>
#include <set>


namespace platypus
{
    template <typename T>
    class MemoryPool
    {
    protected:
        size_t _totalLength = 0;
        size_t _occupiedCount = 0;
        int32_t _highestOccupiedIndex = -1;
        int32_t _prevHighestOccupiedIndex = -1;

        // *Why the fuck isn't _pStorage T* instead?!?!
        void* _pStorage = nullptr;

        // Clearing elements from middle stores the free indices here.
        std::set<size_t> _freeIndices;

    public:
        MemoryPool() {}
        MemoryPool(size_t maxLength);
        MemoryPool(const MemoryPool<T>& other);
        virtual ~MemoryPool();

        // Occupies and constructs element at index
        T* occupy(size_t index);
        // Occupies and constructs element at free index
        // (uses _freeIndices if exists, at back otherwise)
        T* occupy();

        // Clears single element at index and calls its' destructor
        void clearStorage(size_t index);
        // Clears full storage but doesn't resize the actual storage
        // (calls every object's destructor and sets storage to 0)
        void clearStorage();

        // Calls free for _pStorage and sets it to nullptr
        void freeStorage();
        // NOTE: Not sure if this works legally with the updated pool!
        void addSpace(size_t newLength);

        T* operator[](size_t index);
        const T* operator[](size_t index) const;

        // *earlier system called this "first".
        // The "first" was used to find first allocated component of some type
        // to quickly test stuff...
        T* any();

        inline size_t getOccupiedCount() const { return _occupiedCount; }
        inline size_t getTotalLength() const { return _totalLength; }

        inline size_t getOccupiedSize() const { return _occupiedCount * sizeof(T); }
        inline size_t getTotalSize() const { return _totalLength * sizeof(T); }

    private:
        // Returns previous occupied index from index
        // Returns -1 if no previous occupied index found
        int32_t findPreviousOccupiedIndex(size_t index);
    };
}
