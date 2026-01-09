#include "Memory.h"
#include <cstring>
#include <cstdlib>
#include "Debug.h"


namespace platypus
{
    template <typename T>
    MemoryPool<T>::MemoryPool(size_t maxLength) :
        _totalLength(maxLength),
        _occupiedCount(0)
    {
        _pStorage = calloc(_totalLength, sizeof(T));
        memset(_pStorage, 0, getTotalSize());
    }

    // NOTE: Don't remember why I allowed copying?
    template <typename T>
    MemoryPool<T>::MemoryPool(const MemoryPool<T>& other) :
        _totalLength(other._totalLength),
        _occupiedCount(other._occupiedCount),
        _pStorage(other._pStorage)
    {
        Debug::log(
            "Copied memory pool!",
            Debug::MessageType::PLATYPUS_WARNING
        );
        PLATYPUS_ASSERT(false);
    }

    template <typename T>
    MemoryPool<T>::~MemoryPool()
    {
    }

    template <typename T>
    T* MemoryPool<T>::occupy(size_t index)
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
        T* pElement = new (reinterpret_cast<T*>(_pStorage) + index) T;
        ++_occupiedCount;
        return pElement;
    }

    template <typename T>
    T* MemoryPool<T>::occupy()
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

        T* pElement = nullptr;
        if (!_freeIndices.empty())
        {
            size_t index = *_freeIndices.begin();
            pElement = new (reinterpret_cast<T*>(_pStorage) + index) T;
            _freeIndices.erase(index);
            int32_t signedIndex = static_cast<int32_t>(index);
            if (signedIndex > _prevHighestOccupiedIndex)
                _prevHighestOccupiedIndex = signedIndex;
        }
        else
        {
            // If we ever get here all indices MUST be occupied up until _occupiedCount!
            pElement = new (reinterpret_cast<T*>(_pStorage) + _occupiedCount) T;
            _prevHighestOccupiedIndex = _highestOccupiedIndex;
            _highestOccupiedIndex = static_cast<int32_t>(_occupiedCount);
        }
        ++_occupiedCount;
        return pElement;
    }

    template <typename T>
    void MemoryPool<T>::clearStorage(size_t index)
    {
        if (index >= _totalLength)
        {
            Debug::log(
                "Index: " + std::to_string(index) + " out of bounds! "
                "Total allocated elements: " + std::to_string(_totalLength) + " "
                "total allocated size: " + std::to_string(getTotalSize()) + " "
                "element size: " + std::to_string(sizeof(T)),
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

        T* ptr = reinterpret_cast<T*>(_pStorage) + index;
        ptr->~T();
        uint8_t* pData = reinterpret_cast<uint8_t*>(_pStorage) + index;
        memset(reinterpret_cast<void*>(pData), 0, sizeof(T));
        --_occupiedCount;
    }

    template <typename T>
    void MemoryPool<T>::clearStorage()
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
                    T* ptr = reinterpret_cast<T*>(_pStorage) + index;
                    ptr->~T();
                }
            }
        }

        memset(_pStorage, 0, getTotalSize());
        _occupiedCount = 0;
    }

    template <typename T>
    void MemoryPool<T>::freeStorage()
    {
        if (_highestOccupiedIndex != -1)
        {
            size_t lastIndex = static_cast<size_t>(_highestOccupiedIndex);
            for (size_t index = 0; index < lastIndex; ++index)
            {
                if (_freeIndices.find(index) == _freeIndices.end())
                {
                    T* ptr = reinterpret_cast<T*>(_pStorage) + index;
                    ptr->~T();
                }
            }
        }

        free(_pStorage);
        _pStorage = nullptr;

        Debug::log(
            "Freed " + std::to_string(getTotalSize()) + " bytes",
            PLATYPUS_CURRENT_FUNC_NAME
        );

        _totalLength = 0;
        _occupiedCount = 0;
        _prevHighestOccupiedIndex = -1;
        _highestOccupiedIndex = -1;
    }

    // NOTE: Not sure if this works legally with the updated pool!
    template <typename T>
    void MemoryPool<T>::addSpace(size_t newLength)
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

        size_t newSize = newLength * sizeof(T);
        void* pNewStorage = calloc(newLength, sizeof(T));
        memset(pNewStorage, 0, newSize);
        memcpy(pNewStorage, _pStorage, getTotalSize());
        free(_pStorage);
        _pStorage = pNewStorage;
        _totalLength = newLength;
    }

    template <typename T>
    T* MemoryPool<T>::operator[](size_t index)
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

        uint8_t* ptr = reinterpret_cast<uint8_t*>(_pStorage) + index;
        return reinterpret_cast<T*>(ptr);
    }

    template <typename T>
    const T* MemoryPool<T>::operator[](size_t index) const
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

        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(_pStorage) + index;
        return reinterpret_cast<const T*>(ptr);
    }

    template <typename T>
    T* MemoryPool<T>::any()
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
        return reinterpret_cast<T*>((reinterpret_cast<uint8_t*>(_pStorage) + _highestOccupiedIndex));
    }

    template <typename T>
    int32_t MemoryPool<T>::findPreviousOccupiedIndex(size_t index)
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
