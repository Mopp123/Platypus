#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>
#include <map>


namespace platypus
{
    enum class MemoryPoolResizeType
    {
        INCREMENT,
        DOUBLE
    };

    class StaticElementSizeMemoryPool
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
        StaticElementSizeMemoryPool() {}
        StaticElementSizeMemoryPool(
            size_t elementSize,
            size_t maxLength,
            bool allowResize,
            MemoryPoolResizeType resizeType = MemoryPoolResizeType::INCREMENT
        );
        StaticElementSizeMemoryPool(const StaticElementSizeMemoryPool& other);
        virtual ~StaticElementSizeMemoryPool();

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
        // *pUserData can be used to convert from some data type into pool index using userDataToIndex()
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


    class DynamicElementSizeMemoryPool
    {
    private:
        std::vector<uint8_t> _data;
        // key = offset, value = count
        std::map<size_t, size_t> _freeRanges;

        // args:
        // *offset
        // *size
        // *pUserData
        void (*_pFreeRangeFunc)(size_t, size_t, void*) = nullptr;
        void* _pFreeRangeFuncUserData = nullptr;

        bool (*_pValidateFreeRangeFunc)(size_t, size_t, void*) = nullptr;
        void* _pValidateFreeRangeFuncUserData = nullptr;

    public:
        DynamicElementSizeMemoryPool(
            void (*pFreeStorageFunc)(size_t, size_t, void*),
            void* pFreeStorageFuncUserData,
            bool (*pValidateFreeRangeFunc)(size_t, size_t, void*),
            void* pValidateFreeRangeFuncUserData
        );

        // Returns the offset where child entities begin in _childrenContainer or
        // -1 if fails to occupy
        int32_t occupyRange(size_t dataSize, const void* pData);
        void freeRange(int32_t offset, size_t size);

        // Changes the previously used offset and returns it
        // TODO: add helper func for Children component that sets the new offset and child count
        int32_t add(
            int32_t baseOffset,
            size_t currentSize,
            size_t addedDataSize,
            const void* pData
        );

        // NOTE: Shouldn't be needed since having freeRange func!
        // TODO: Remove?
        // TODO: add helper func for Children component that decreases the child count
        //void remove(int32_t elementOffset, size_t elementSize);

        const void* accessData(int32_t offset, size_t size) const;
        std::vector<uint8_t> copyData(int32_t offset, size_t size) const;

        // NOTE: Not tested after latest changes! MIGHT NOT WORK PROPERLY!!
        // NOTE: this is too complicated, inefficient and dumb
        // TODO: Improve, optimize ..or something...
        void packFreeRanges();

        inline std::vector<uint8_t>& accessStorage() { return _data; }
        inline const std::vector<uint8_t>& getStorage() const { return _data; }
        inline std::map<size_t, size_t>& accessFreeRanges() { return _freeRanges; }
        inline const std::map<size_t, size_t>& getFreeRanges() const { return _freeRanges; }

        inline size_t getTotalSize() const { return _data.size(); }

    private:
        int32_t findFreeRange(size_t requiredSize, size_t& outAvailableSize);
        bool validateFreeRange(size_t offset, size_t size) const;
    };
}
