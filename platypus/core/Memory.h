#pragma once

#include <cstddef>
#include <cstdint>
#include <set>


namespace platypus
{
    class MemoryPool
    {
    protected:
        size_t _elementSize = 0;
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
        MemoryPool(size_t elementSize, size_t maxLength);
        MemoryPool(const MemoryPool& other);
        virtual ~MemoryPool();

        virtual void constructElement(size_t index, void* pData, void* pUserData) = 0;
        virtual void destroyElement(size_t index, void* pData, void* pUserData) = 0;
        virtual void onClearFullStorage() = 0;
        virtual void onFreeStorage() = 0;

        // NOTE: Shouldn't really even be used!?
        // Occupies and constructs element at index.
        // Returns ptr to the constructed element
        //void* occupy(size_t index);

        // Occupies and constructs element at first found free index.
        // Returns ptr to the constructed element
        // Returns nullptr if fails to occupy any index
        // (uses _freeIndices if exists, at back otherwise)
        void* occupy(void* pUserData);

        // Clears single element at index and calls its' destructor
        void clearStorage(size_t index, void* pUserData);
        // Clears full storage but doesn't resize the actual storage
        // (calls every object's destructor and sets storage to 0)
        void clearStorage();

        // Calls free for _pStorage and sets it to nullptr
        void freeStorage();

        // NOTE: Not sure if this works legally with the updated pool!
        void addSpace(size_t newLength);

        void* operator[](size_t index);
        const void* operator[](size_t index) const;

        // *earlier system called this "first".
        // The "first" was used to find first allocated component of some type
        // to quickly test stuff...
        void* any();

        inline size_t getOccupiedCount() const { return _occupiedCount; }
        inline size_t getTotalLength() const { return _totalLength; }

        inline size_t getOccupiedSize() const { return _occupiedCount * _elementSize; }
        inline size_t getTotalSize() const { return _totalLength * _elementSize; }

    private:
        // Returns previous occupied index from index
        // Returns -1 if no previous occupied index found
        int32_t findPreviousOccupiedIndex(size_t index);
    };
}
