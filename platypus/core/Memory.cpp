#include "Memory.hpp"
#include "Debug.hpp"
#include "platypus/ecs/Entity.hpp"
#include <cstring>
#include <cstdlib>
#include <limits>


namespace platypus
{
    StaticElementSizeMemoryPool::StaticElementSizeMemoryPool(
        size_t elementSize,
        size_t maxLength,
        bool allowResize,
        MemoryPoolResizeType resizeType
    ) :
        _elementSize(elementSize),
        _totalLength(maxLength),
        _occupiedCount(0),
        _allowResize(allowResize),
        _resizeType(resizeType)
    {
        // Some member funcs returns either valid index or -1 to the allocated space.
        // int32_t is atm used, so length can't exceed max value of int32_t
        constexpr int32_t maxInt32_t{std::numeric_limits<int32_t>::max()};
        if (maxLength > maxInt32_t)
        {
            Debug::log(
                "Pool length can't exceed maximum value of int32_t(" + std::to_string(maxInt32_t) + ". "
                "Requested length was " + std::to_string(maxLength),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _pStorage = calloc(_totalLength, _elementSize);
        memset(_pStorage, 0, getTotalSize());
    }

    // NOTE: Don't remember why I allowed copying?
    StaticElementSizeMemoryPool::StaticElementSizeMemoryPool(const StaticElementSizeMemoryPool& other) :
        _elementSize(other._elementSize),
        _totalLength(other._totalLength),
        _occupiedCount(other._occupiedCount),
        _allowResize(other._allowResize),
        _resizeType(other._resizeType),
        _pStorage(other._pStorage)
    {
        Debug::log(
            "Copied memory pool!",
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_WARNING
        );
        PLATYPUS_ASSERT(false);
    }

    StaticElementSizeMemoryPool::~StaticElementSizeMemoryPool()
    {
    }

    void* StaticElementSizeMemoryPool::occupy(void* pUserData)
    {
        if (_occupiedCount >= _totalLength)
        {
            if (!_allowResize)
            {
                Debug::log(
                    "Pool was full and resizing wasn't enabled",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return nullptr;
            }
            size_t newLength = _totalLength + 1;
            if (_resizeType == MemoryPoolResizeType::DOUBLE)
                newLength = _totalLength * 2;

            addSpace(newLength);
        }

        void* pElement = nullptr;
        size_t occupyIndex = 0;
        if (!_freeIndices.empty())
        {
            occupyIndex = *_freeIndices.begin();
            uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + occupyIndex * _elementSize;
            pElement = reinterpret_cast<void*>(ptr);
            _freeIndices.erase(occupyIndex);
            int32_t signedIndex = static_cast<int32_t>(occupyIndex);
            // NOTE: There were some issues with the highest occupied index...
            // ...not sure if I trust this system anymore...
            if (signedIndex > _prevHighestOccupiedIndex && signedIndex < _highestOccupiedIndex)
            {
                _prevHighestOccupiedIndex = signedIndex;
            }
            else if (signedIndex > _highestOccupiedIndex)
            {
                _prevHighestOccupiedIndex = _highestOccupiedIndex;
                _highestOccupiedIndex = signedIndex;
            }
        }
        else
        {
            // If we ever get here all indices MUST be occupied up until _occupiedCount!
            occupyIndex = _occupiedCount;
            uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + occupyIndex * _elementSize;
            pElement = reinterpret_cast<void*>(ptr);
            _prevHighestOccupiedIndex = _highestOccupiedIndex;
            _highestOccupiedIndex = static_cast<int32_t>(occupyIndex);
        }
        constructElement(occupyIndex, pElement, pUserData);
        ++_occupiedCount;
        return pElement;
    }

    // NOTE: Changed a bit after some fuckery...
    // NOT TESTED! USE THE OTHER ONE INSTEAD IF POSSIBLE!
    void StaticElementSizeMemoryPool::clearStorage(size_t index, void* pUserData)
    {
        if (index >= _totalLength)
        {
            Debug::log(
                "Index: " + std::to_string(index) + " out of bounds! "
                "Total allocated elements: " + std::to_string(_totalLength) + " "
                "total allocated size: " + std::to_string(getTotalSize()) + " "
                "element size: " + std::to_string(_elementSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // Add to freed indices so next occupation can use that instead of "back"
        // +also fuck around with the highest occupied index...
        if (index == static_cast<size_t>(_highestOccupiedIndex))
        {
            if (_prevHighestOccupiedIndex != -1)
            {
                _highestOccupiedIndex = _prevHighestOccupiedIndex;
                if (_highestOccupiedIndex != -1)
                    _prevHighestOccupiedIndex = findPreviousOccupiedIndex(_highestOccupiedIndex);
            }
            else
            {
                // If we get here it means there should not be a single occupied index left?
                _highestOccupiedIndex = -1;
            }
        }
        else if (index == static_cast<size_t>(_prevHighestOccupiedIndex))
        {
            _prevHighestOccupiedIndex = findPreviousOccupiedIndex(index);
        }
        // NOTE: For some reason didn't previously add to _freeIndices if freeing the last elem...
        // ...Don't remember why..
        _freeIndices.insert(index);

        uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index * _elementSize;
        void* voidPtr = reinterpret_cast<void*>(ptr);
        destroyElement(index, voidPtr, pUserData);
        memset(voidPtr, 0, _elementSize);
        --_occupiedCount;
    }

    void StaticElementSizeMemoryPool::clearStorage(void* pUserData)
    {
        int32_t signedIndex = userDataToIndex(pUserData);
        if (signedIndex == -1)
        {
            Debug::log(
                "Failed to convert pUserData to pool index",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        size_t index = static_cast<size_t>(signedIndex);
        if (index >= _totalLength)
        {
            Debug::log(
                "Index: " + std::to_string(index) + " out of bounds! "
                "Total allocated elements: " + std::to_string(_totalLength) + " "
                "total allocated size: " + std::to_string(getTotalSize()) + " "
                "element size: " + std::to_string(_elementSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // Add to freed indices so next occupation can use that instead of "back"
        // +also fuck around with the highest occupied index...
        if (index == static_cast<int32_t>(_highestOccupiedIndex))
        {
            if (_prevHighestOccupiedIndex != -1)
            {
                _highestOccupiedIndex = _prevHighestOccupiedIndex;
                if (_highestOccupiedIndex != -1)
                    _prevHighestOccupiedIndex = findPreviousOccupiedIndex(_highestOccupiedIndex);
            }
            else
            {
                // If we get here it means there should not be a single occupied index left?
                _highestOccupiedIndex = -1;
            }
        }
        else if (index == static_cast<int32_t>(_prevHighestOccupiedIndex))
        {
            _prevHighestOccupiedIndex = findPreviousOccupiedIndex(index);
        }
        // NOTE: For some reason didn't previously add to _freeIndices if freeing the last elem...
        // ...Don't remember why..
        _freeIndices.insert(index);

        uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index * _elementSize;
        void* voidPtr = reinterpret_cast<void*>(ptr);
        destroyElement(index, voidPtr, pUserData);
        memset(voidPtr, 0, _elementSize);
        --_occupiedCount;
    }

    void StaticElementSizeMemoryPool::clearStorage()
    {
        if (_occupiedCount == 0)
        {
            Debug::log(
                "Pool's occupied length was already 0",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        if (_highestOccupiedIndex != -1)
        {
            size_t lastIndex = static_cast<size_t>(_highestOccupiedIndex);
            for (size_t index = 0; index < lastIndex; ++index)
            {
                if (_freeIndices.find(index) == _freeIndices.end())
                {
                    uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index * _elementSize;
                    destroyElement(index, reinterpret_cast<void*>(ptr), nullptr);
                }
            }
        }

        onClearFullStorage();
        memset(_pStorage, 0, getTotalSize());
        _occupiedCount = 0;
    }

    void StaticElementSizeMemoryPool::freeStorage()
    {
        if (_highestOccupiedIndex != -1)
        {
            size_t lastIndex = static_cast<size_t>(_highestOccupiedIndex);
            for (size_t index = 0; index < lastIndex; ++index)
            {
                if (_freeIndices.find(index) == _freeIndices.end())
                {
                    uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index * _elementSize;
                    destroyElement(index, reinterpret_cast<void*>(ptr), nullptr);
                }
            }
        }

        onFreeStorage();

        free(_pStorage);
        _pStorage = nullptr;

        Debug::log(
            "Freed " + std::to_string(getTotalSize()) + " bytes",
            PLATYPUS_CURRENT_FUNC_NAME
        );

        _elementSize = 0;
        _totalLength = 0;
        _occupiedCount = 0;
        _prevHighestOccupiedIndex = -1;
        _highestOccupiedIndex = -1;
    }

    // NOTE: Not sure if this works legally with the updated pool!
    void StaticElementSizeMemoryPool::addSpace(size_t newLength)
    {
        if (newLength < _totalLength)
        {
            Debug::log(
                "New length: " + std::to_string(newLength) + " was less "
                "than current length: " + std::to_string(_totalLength),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        size_t newSize = newLength * _elementSize;
        void* pNewStorage = calloc(newLength, _elementSize);
        memset(pNewStorage, 0, newSize);
        memcpy(pNewStorage, _pStorage, getTotalSize());
        free(_pStorage);
        _pStorage = pNewStorage;
        _totalLength = newLength;
    }

    void* StaticElementSizeMemoryPool::getElement(void* pUserData)
    {
        int32_t signedIndex = userDataToIndex(pUserData);
        if (signedIndex == -1)
            return nullptr;

        size_t index = static_cast<size_t>(signedIndex);
        #ifdef PLATYPUS_DEBUG
        if (index > _highestOccupiedIndex)
        {
            entityID_t entity;
            memcpy(reinterpret_cast<void*>(&entity), pUserData, sizeof(entityID_t));
            Debug::log(
                "Entity's: " +std::to_string(entity) + " Index: " + std::to_string(index) + " out of bounds of occupied indices! "
                "Highest occupied index was " + std::to_string(_highestOccupiedIndex),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_freeIndices.find(index) != _freeIndices.end())
        {
            Debug::log(
                "Element at index " + std::to_string(index) + " was already freed!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        #endif

        uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index * _elementSize;
        return reinterpret_cast<void*>(ptr);
    }

    const void* StaticElementSizeMemoryPool::getElement(void* pUserData) const
    {
        int32_t signedIndex = userDataToIndex(pUserData);
        if (signedIndex == -1)
            return nullptr;

        size_t index = static_cast<int32_t>(signedIndex);
        #ifdef PLATYPUS_DEBUG
        if (index > _highestOccupiedIndex)
        {
            Debug::log(
                "Index: " + std::to_string(index) + " out of bounds of occupied indices!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_freeIndices.find(index) != _freeIndices.end())
        {
            Debug::log(
                "Element at index " + std::to_string(index) + " was already freed!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        #endif

        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(_pStorage) + index * _elementSize;
        return reinterpret_cast<const void*>(ptr);
    }

    void* StaticElementSizeMemoryPool::any()
    {
        if (_highestOccupiedIndex == -1)
        {
            Debug::log(
                "No occupied elements exist!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return reinterpret_cast<void*>(
            (reinterpret_cast<uint8_t*>(_pStorage) + _highestOccupiedIndex * _elementSize)
        );
    }

    int32_t StaticElementSizeMemoryPool::findPreviousOccupiedIndex(size_t index)
    {
        if (index > _totalLength)
        {
            Debug::log(
                "Index " + std::to_string(index) + " "
                "exceeded max length of " + std::to_string(_totalLength),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return 0;
        }

        int32_t signedIndex = static_cast<int32_t>(index);
        for (int32_t currentIndex = signedIndex - 1; currentIndex >= 0; --currentIndex)
        {
            if (_freeIndices.find(static_cast<size_t>(currentIndex)) == _freeIndices.end())
                return currentIndex;
        }
        return -1;
    }


    DynamicElementSizeMemoryPool::DynamicElementSizeMemoryPool(
        void (*pFreeRangeFunc)(size_t, size_t, void*),
        void* pFreeRangeFuncUserData,
        bool (*pValidateFreeRangeFunc)(size_t, size_t, void*),
        void* pValidateFreeRangeFuncUserData
    ) :
        _pFreeRangeFunc(pFreeRangeFunc),
        _pFreeRangeFuncUserData(pFreeRangeFuncUserData),
        _pValidateFreeRangeFunc(pValidateFreeRangeFunc),
        _pValidateFreeRangeFuncUserData(pValidateFreeRangeFuncUserData)
    {
    }

    int32_t DynamicElementSizeMemoryPool::occupyRange(size_t dataSize, const void* pData)
    {
        // Check first if suitable free range already exists
        int32_t offset = findFreeRange(dataSize);

        if (offset == -1)
        {
            const size_t prevSize = _data.size();
            _data.resize(prevSize + dataSize);
            memcpy(
                _data.data() + prevSize,
                pData,
                dataSize
            );
            offset = prevSize;
        }
        else
        {
            #ifdef PLATYPUS_DEBUG
            if (!validateFreeRange(offset, dataSize))
            {
                Debug::log(
                    "Free range validation failed using offset: " + std::to_string(offset) + " and size: " + std::to_string(dataSize) + " "
                    "Storage size is " + std::to_string(_data.size()),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            memcpy(
                _data.data() + offset,
                pData,
                sizeof(entityID_t) * dataSize
            );
            _freeRanges.erase(offset);
        }

        return offset;
    }

    void DynamicElementSizeMemoryPool::freeRange(int32_t offset, size_t size)
    {
        PLATYPUS_ASSERT(offset >= 0);
        if (offset + size > _data.size())
        {
            Debug::log(
                "Range (offset = " + std::to_string(offset) + " size = " + std::to_string(size) + ") "
                "out of bounds! Storage size is " + std::to_string(_data.size()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        const size_t unsignedOffset = static_cast<size_t>(offset);
        PLATYPUS_ASSERT(_pFreeRangeFunc);
        _pFreeRangeFunc(offset, size, _pFreeRangeFuncUserData);
        //for (size_t i = unsignedOffset; i < unsignedOffset + count; ++i)
        //    _childrenContainer[i] = NULL_ENTITY_ID;

        _freeRanges[unsignedOffset] = size;

        packFreeRanges();
    }

    int32_t DynamicElementSizeMemoryPool::add(
        int32_t baseOffset,
        size_t currentSize,
        size_t addedDataSize,
        const void* pData
    )
    {
        if (baseOffset == -1)
            return occupyRange(addedDataSize, pData);

        // Quickly return using same baseOffset if just adding at the back of the container
        if (baseOffset + currentSize == _data.size())
        {
            const size_t prevSize = _data.size();
            _data.resize(prevSize + addedDataSize);
            memcpy(_data.data() + baseOffset + currentSize, pData, addedDataSize);
            return baseOffset;
        }

        // Quickly return using same baseOffset if can add at empty pos after current range
        // NOTE: BELOW QUITE COMPLICATED, NOT TESTED MIGHT BE FUCKED!!
        // TODO: TEST PROPERLY!
        if (currentSize > 0)
        {
            const size_t nextOffset = baseOffset + currentSize;
            std::map<size_t, size_t>::const_iterator freeIt = _freeRanges.find(nextOffset);
            // Can add at least one more if found from _freeRanges
            if (freeIt != _freeRanges.end())
            {
                const size_t freeSize = freeIt->second;
                // TODO:
                //  *If free range found at the end of the storage but the storage's and free
                //  range's size isn't enough, use the free offset and alloc the remaining
                //  required space!
                //  *Test that!
                CONTINUE HERE
                if (freeSize >= addedDataSize)
                {
                    PLATYPUS_ASSERT(freeIt->first < _data.size());
                    // TODO: Make this kind of "null elem check" possible
                    //PLATYPUS_ASSERT(_childrenContainer[nextOffset] == NULL_ENTITY_ID);
                    //_childrenContainer[nextOffset] = childEntityID;
                    memcpy(_data.data() + nextOffset, pData, addedDataSize);

                    _freeRanges.erase(nextOffset);
                    // Update the free offsets
                    // If theres more space after the old free baseOffset, "push the cursor forward"
                    // with the new free count
                    if (freeSize > addedDataSize)
                    {
                        const size_t newFreeSize = freeSize - addedDataSize;
                        const size_t newFreeOffset = nextOffset + addedDataSize;
                        if (newFreeOffset < _data.size())
                            _freeRanges[newFreeOffset] = newFreeSize;
                    }

                    return baseOffset;
                }
            }
        }

        PLATYPUS_ASSERT(baseOffset >= 0);

        std::vector<uint8_t> currentData = copyData(baseOffset, currentSize);
        freeRange(baseOffset, currentSize);
        currentData.resize(currentSize + addedDataSize);
        memcpy(currentData.data() + currentSize, pData, addedDataSize);

        return occupyRange(currentData.size(), currentData.data());
    }

    // TODO: Remove?
    /*
    void DynamicElementSizeMemoryPool::remove(int32_t elementOffset, size_t elementSize)
    {
        const int32_t currentOffset = pChildren->offset;
        const size_t currentCount = pChildren->count;
        PLATYPUS_ASSERT(currentOffset >= 0);

        const size_t unsignedCurrentOffset = static_cast<const size_t>(currentOffset);
        const size_t end = unsignedCurrentOffset + currentCount;
        PLATYPUS_ASSERT(end <= _childrenContainer.size());
        for (size_t i = unsignedCurrentOffset; i < end; ++i)
        {
            const entityID_t entityID = _childrenContainer[i];
            if (entityID == childEntityID)
            {
                _childrenContainer[i] = NULL_ENTITY_ID;
                if (i == _childrenContainer.size() - 1)
                {
                    _childrenContainer.pop_back();
                    return;
                }

                // Make all the rest of the child entities IDs be contiguous
                for (size_t j = i; j < end; ++j)
                {
                    if (j + 1 >= end)
                        break;

                    _childrenContainer[j] = _childrenContainer[j + 1];
                }
                _freeRanges[end - 1] = 1;
                packFreeRanges();
                return;
            }
        }

        Debug::log(
            "Failed to find entityID " + std::to_string(childEntityID) + " "
            "from range: " + std::to_string(currentOffset) + " to " + std::to_string(currentOffset + currentCount),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }
    */

    const void* DynamicElementSizeMemoryPool::accessData(int32_t offset, size_t size) const
    {
        if (offset + size > _data.size())
        {
            Debug::log(
                "Range (offset = " + std::to_string(offset) + " size = " + std::to_string(size) + ") "
                "out of bounds! Storage size is " + std::to_string(_data.size()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _data.data() + offset;
    }

    std::vector<uint8_t> DynamicElementSizeMemoryPool::copyData(int32_t offset, size_t size) const
    {
        const void* pData = accessData(offset, size);
        if (!pData)
            return { };

        std::vector<uint8_t> data(size);
        memcpy(data.data(), pData, size);
        return data;
    }

    // NOTE: Not tested after latest changes! MIGHT NOT WORK PROPERLY!!
    void DynamicElementSizeMemoryPool::packFreeRanges()
    {
        if (_freeRanges.empty())
            return;

        std::map<size_t, size_t> result;
        std::map<size_t, size_t>::iterator currentIt = _freeRanges.begin();
        std::map<size_t, size_t>::iterator nextIt = currentIt;

        size_t currentOffset = currentIt->first;
        size_t currentSize = currentIt->second;
        size_t currentEndOffset = currentOffset + currentSize; // *past the last offset
        size_t nextIterIncr = 1;
        while (true)
        {
            ++nextIt;
            if (nextIt == _freeRanges.end())
                break;

            const size_t nextOffset = nextIt->first;
            const size_t nextSize = nextIt->second;
            // If next beginst right after current
            //  -> merge the next to the current
            if (currentEndOffset == nextOffset)
            {
                const size_t newSize = currentSize + nextSize;
                result[currentIt->first] = newSize;
                currentSize = newSize;
                currentEndOffset = currentOffset + currentSize;
                ++nextIterIncr;
            }
            else
            {
                result[currentIt->first] = currentSize;
                result[nextIt->first] = nextSize;
                for (size_t i = 0; i < nextIterIncr; ++i)
                    ++currentIt;

                nextIterIncr = 1;
                currentOffset = currentIt->first;
                currentSize = currentIt->second;
                currentEndOffset = currentOffset + currentSize;
            }
        }
        _freeRanges = result;
    }

    int32_t DynamicElementSizeMemoryPool::findFreeRange(size_t requiredCount)
    {
        std::map<size_t, size_t>::const_iterator it;
        for (it = _freeRanges.begin(); it != _freeRanges.end(); ++it)
        {
            if (it->second >= requiredCount)
                return it->first;
        }
        return -1;
    }

    bool DynamicElementSizeMemoryPool::validateFreeRange(size_t offset, size_t size) const
    {
        if (offset + size > _data.size())
            return false;

        PLATYPUS_ASSERT(_pValidateFreeRangeFunc);
        return _pValidateFreeRangeFunc(offset, size, _pValidateFreeRangeFuncUserData);
    }
}
