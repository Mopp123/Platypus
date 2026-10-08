#include "StringPool.hpp"
#include "Debug.hpp"
#include <cstring>


namespace platypus
{
    int32_t StringPool::add(const std::string& str)
    {
        StoredString stored = toStoredString(str);
        return _memoryPool.add(
            -1, // current offset
            0, // current size
            stored.totalSize,
            &stored
        );
    }

    int32_t StringPool::update(int32_t offset, const std::string& newStr)
    {
        if (offset == - 1)
        {
            Debug::log(
                "Offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return { };
        }
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (offset >= totalStorageSize)
        {
            Debug::log(
                "Offset " + std::to_string(offset) + " out of bounds! "
                "MemoryPool size is " + std::to_string(totalStorageSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return { };
        }

        const uint8_t* pStorage = _memoryPool.getStorage().data();

        uint32_t totalStoredSize = 0;
        memcpy(&totalStoredSize, pStorage + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        PLATYPUS_ASSERT(offset < totalStorageSize);

        uint32_t dataSize = 0;
        memcpy(&dataSize, pStorage + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        PLATYPUS_ASSERT(offset < totalStorageSize);
        PLATYPUS_ASSERT(offset + dataSize <= totalStorageSize);

        std::string strCpy = std::string(
            reinterpret_cast<const char*>(pStorage + offset),
            static_cast<size_t>(dataSize)
        );

        remove(offset);
        return add(strCpy);
    }


    void StringPool::remove(int32_t offset)
    {
        if (offset == - 1)
        {
            Debug::log(
                "Offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (offset >= totalStorageSize)
        {
            Debug::log(
                "Offset " + std::to_string(offset) + " out of bounds! "
                "MemoryPool size is " + std::to_string(totalStorageSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        const uint8_t* pStorage = _memoryPool.getStorage().data();

        uint32_t totalStoredSize = 0;
        memcpy(&totalStoredSize, pStorage + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        PLATYPUS_ASSERT(offset < totalStorageSize);

        _memoryPool.freeRange(offset, totalStoredSize);
    }

    std::string StringPool::get(int32_t offset) const
    {
        if (offset == - 1)
        {
            Debug::log(
                "Offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return "";
        }
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (offset >= totalStorageSize)
        {
            Debug::log(
                "Offset " + std::to_string(offset) + " out of bounds! "
                "MemoryPool size is " + std::to_string(totalStorageSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return "";
        }

        const uint8_t* pStorage = _memoryPool.getStorage().data();

        uint32_t totalStoredSize = 0;
        memcpy(&totalStoredSize, pStorage + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        PLATYPUS_ASSERT(offset < totalStorageSize);

        uint32_t dataSize = 0;
        memcpy(&dataSize, pStorage + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        PLATYPUS_ASSERT(offset < totalStorageSize);
        PLATYPUS_ASSERT(offset + dataSize <= totalStorageSize);

        return std::string(
            reinterpret_cast<const char*>(pStorage + offset),
            static_cast<size_t>(dataSize)
        );
    }

    StringPool::StoredString StringPool::toStoredString(const std::string& str) const
    {
        const uint32_t baseSize = sizeof(uint32_t) * 2;
        const uint32_t dataSize = str.size();
        const uint32_t totalSize = baseSize + dataSize;
        return {
            totalSize,
            dataSize,
            str.data()
        };
    }
}
