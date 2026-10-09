#include "StringPool.hpp"
#include "Debug.hpp"
#include <cstring>


namespace platypus
{
    StringPool::StringPool() :
        _memoryPool(
            free_storage_func,
            this,
            validate_free_range_func,
            this
        )
    {
    }

    int32_t StringPool::add(const std::string& str)
    {
        const uint32_t totalSize = sizeof(uint32_t) * 2 + str.size();
        const uint32_t dataSize = str.size();
        std::vector<uint8_t> storedStr(totalSize);
        uint8_t* pStoredStr = storedStr.data();
        memcpy(pStoredStr, &totalSize, sizeof(uint32_t));
        memcpy(pStoredStr + sizeof(uint32_t), &dataSize, sizeof(uint32_t));
        memcpy(pStoredStr + sizeof(uint32_t) * 2, str.data(), dataSize);

        return _memoryPool.add(
            -1, // current offset
            0, // current size
            totalSize,
            pStoredStr
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

        /*
        const uint8_t* pStorage = _memoryPool.getStorage().data();

        uint32_t totalStoredSize = 0;
        size_t dataOffset = static_cast<size_t>(offset);
        memcpy(&totalStoredSize, pStorage + dataOffset, sizeof(uint32_t));
        dataOffset += sizeof(uint32_t);
        PLATYPUS_ASSERT(dataOffset < totalStorageSize);

        uint32_t dataSize = 0;
        memcpy(&dataSize, pStorage + dataOffset, sizeof(uint32_t));
        dataOffset += sizeof(uint32_t);
        PLATYPUS_ASSERT(dataOffset < totalStorageSize);
        PLATYPUS_ASSERT(dataOffset + dataSize <= totalStorageSize);

        std::string strCpy = std::string(
            reinterpret_cast<const char*>(pStorage + dataOffset),
            static_cast<size_t>(dataSize)
        );
        */

        remove(offset);
        return add(newStr);
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
        size_t dataOffset = static_cast<size_t>(offset);
        memcpy(&totalStoredSize, pStorage + dataOffset, sizeof(uint32_t));
        dataOffset += sizeof(uint32_t);
        PLATYPUS_ASSERT(dataOffset < totalStorageSize);

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

    std::vector<std::string> StringPool::getAll() const
    {
        const std::vector<uint8_t> storage = _memoryPool.getStorage();
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (storage.empty())
            return { };

        const uint8_t* pStorage = storage.data();
        const std::map<size_t, size_t>& freeRanges = _memoryPool.getFreeRanges();
        std::vector<std::string> result;
        size_t offset = 0;
        while (offset < totalStorageSize)
        {
            std::map<size_t, size_t>::const_iterator freeIt = freeRanges.find(offset);
            if (freeIt != freeRanges.end())
            {
                offset += freeIt->second;
                continue;
            }

            // NOTE: Should we allow totalSize == sizeof(uint32_t) * 2
            //  -> meaning we have stored empty string!?
            uint32_t totalSize = 0;
            memcpy(&totalSize, pStorage + offset, sizeof(uint32_t));
            // NOTE: If offset not marked as free, it should contain valid string, if not at the
            // end of the storage!?
            PLATYPUS_ASSERT(totalSize > 0 && totalSize <= totalStorageSize);
            offset += sizeof(uint32_t);
            PLATYPUS_ASSERT(offset < totalStorageSize);

            uint32_t strSize = 0;
            memcpy(&strSize, pStorage + offset, sizeof(uint32_t));
            PLATYPUS_ASSERT(strSize > 0);
            offset += sizeof(uint32_t);
            PLATYPUS_ASSERT(offset < totalStorageSize);

            std::string str = std::string(
                reinterpret_cast<const char*>(pStorage + offset),
                strSize
            );
            result.push_back(str);
            offset += strSize;
        }
        return result;
    }

    void StringPool::free_storage_func(size_t offset, size_t size, void* pUserData)
    {
        StringPool* pStringPool = reinterpret_cast<StringPool*>(pUserData);
        DynamicElementSizeMemoryPool& memoryPool = pStringPool->_memoryPool;
        PLATYPUS_ASSERT(offset + size <= memoryPool.getTotalSize());
        memset(memoryPool.accessStorage().data() + offset, 0, size);
    }

    bool StringPool::validate_free_range_func(size_t offset, size_t size, void* pUserData)
    {
        const StringPool* pStringPool = reinterpret_cast<const StringPool*>(pUserData);
        const DynamicElementSizeMemoryPool& memoryPool = pStringPool->_memoryPool;
        PLATYPUS_ASSERT(offset + size <= memoryPool.getTotalSize());
        std::vector<uint8_t> emptyBytes(size);
        memset(emptyBytes.data(), 0, size);
        return memcmp(memoryPool.getStorage().data() + offset, emptyBytes.data(), size) == 0;
    }
}
