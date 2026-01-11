#include "Memory.h"
#include <cstring>
#include <cstdlib>
#include <limits>
#include "Debug.h"


namespace platypus
{
    MemoryPool::MemoryPool(size_t elementSize, size_t maxLength) :
        _elementSize(elementSize),
        _totalLength(maxLength),
        _occupiedCount(0)
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
    MemoryPool::MemoryPool(const MemoryPool& other) :
        _elementSize(other._elementSize),
        _totalLength(other._totalLength),
        _occupiedCount(other._occupiedCount),
        _pStorage(other._pStorage)
    {
        Debug::log(
            "Copied memory pool!",
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_WARNING
        );
        PLATYPUS_ASSERT(false);
    }

    MemoryPool::~MemoryPool()
    {
    }

    /*
    void* MemoryPool::occupy(size_t index)
    {
        if (index >= _totalLength)
        {
            Debug::log(
                "Index " + std::to_string(index) + " "
                "exceeded max length of " + std::to_string(_totalLength),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index * _elementSize;
        void* voidPtr = reinterpret_cast<void*>(ptr);
        constructElement(voidPtr);
        ++_occupiedCount;
        return voidPtr;
    }
    */

    void* MemoryPool::occupy(void* pUserData)
    {
        if (_occupiedCount >= _totalLength)
        {
            Debug::log(
                "Max length(" + std::to_string(_totalLength) + ") exceeded!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
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
            if (signedIndex > _prevHighestOccupiedIndex)
                _prevHighestOccupiedIndex = signedIndex;
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

    void MemoryPool::clearStorage(size_t index, void* pUserData)
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

        // If clearing from somewhere else than back, add to freed indices
        // so the next occupation can use the free index instead of back
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
            _freeIndices.insert(index);
        }
        else if (index == static_cast<int32_t>(_prevHighestOccupiedIndex))
        {
            _prevHighestOccupiedIndex = findPreviousOccupiedIndex(index);
            _freeIndices.insert(index);
        }

        uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index;
        void* voidPtr = reinterpret_cast<void*>(ptr);
        destroyElement(index, voidPtr, pUserData);
        memset(voidPtr, 0, _elementSize);
        --_occupiedCount;
    }

    void MemoryPool::clearStorage()
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

    void MemoryPool::freeStorage()
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
    void MemoryPool::addSpace(size_t newLength)
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

    void* MemoryPool::operator[](size_t index)
    {
        #ifdef PLATYPUS_DEBUG
        if (index >= _highestOccupiedIndex)
        {
            Debug::log(
                "Index: " + std::to_string(index) + " out of bounds of occupied indices!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_freeIndices.find(index) == _freeIndices.end())
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

    const void* MemoryPool::operator[](size_t index) const
    {
        #ifdef PLATYPUS_DEBUG
        if (index >= _highestOccupiedIndex)
        {
            Debug::log(
                "Index: " + std::to_string(index) + " out of bounds of occupied indices!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_freeIndices.find(index) == _freeIndices.end())
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

    void* MemoryPool::any()
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
            (reinterpret_cast<uint8_t*>(_pStorage) + _highestOccupiedIndex)
        );
    }

    int32_t MemoryPool::findPreviousOccupiedIndex(size_t index)
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

        for (size_t currentIndex = index - 1; currentIndex >= 0; ++currentIndex)
        {
            if (_freeIndices.find(currentIndex) == _freeIndices.end())
                return currentIndex;
        }
        return -1;
    }
}
