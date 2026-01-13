#pragma once

#include <cstddef>
#include <cstdint>
#include <set>


namespace platypus
{
    enum class MemoryPoolResizeType
    {
        INCREMENT,
        DOUBLE
    };

    class MemoryPool
    {
    protected:
        size_t _elementSize = 0;
        size_t _totalLength = 0;
        size_t _occupiedCount = 0;
        int32_t _highestOccupiedIndex = -1;
        int32_t _prevHighestOccupiedIndex = -1;

        bool _allowResize = false;
        MemoryPoolResizeType _resizeType = MemoryPoolResizeType::INCREMENT;

        // *Why the fuck isn't _pStorage T* instead?!?!
        void* _pStorage = nullptr;

        // Clearing elements from middle stores the free indices here.
        std::set<size_t> _freeIndices;

    public:
        MemoryPool() {}
        MemoryPool(
            size_t elementSize,
            size_t maxLength,
            bool allowResize,
            MemoryPoolResizeType resizeType = MemoryPoolResizeType::INCREMENT
        );
        MemoryPool(const MemoryPool& other);
        virtual ~MemoryPool();

        virtual int32_t userDataToIndex(void* pUserData) const = 0;
        virtual void constructElement(size_t index, void* pData, void* pUserData) = 0;
        virtual void destroyElement(size_t index, void* pData, void* pUserData) = 0;
        virtual void onClearFullStorage() = 0;
        virtual void onFreeStorage() = 0;

        // Occupies and constructs element at first found free index.
        // Returns ptr to the constructed element
        // Returns nullptr if fails to occupy any index
        // (uses _freeIndices if exists, at back otherwise)
        void* occupy(void* pUserData);

        // Clears single element at index and calls its' destructor
        // *pUserData can be used to convert from some data type into pool index using userDataToIndex
        void clearStorage(size_t index, void* pUserData);
        void clearStorage(void* pUserData);

        // Clears full storage but doesn't resize the actual storage
        // (calls every object's destructor and sets storage to 0)
        void clearStorage();

        // Calls free for _pStorage and sets it to nullptr
        void freeStorage();

        // NOTE: Not sure if this works legally with the updated pool!
        void addSpace(size_t newLength);

        // *requiers userDataToIndex impl
        void* getElement(void* pUserData);
        const void* getElement(void* pUserData) const;

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
