#pragma once

#include "Memory.hpp"
#include <string>


namespace platypus
{
    class StringPool
    {
    private:
        // Stored string layout:
        //  uint32_t totalSize
        //  uint32_t dataSize
        //  uint8_t data[dataSize]
        DynamicElementSizeMemoryPool _memoryPool;

    public:
        StringPool();
        int32_t add(const std::string& str);
        int32_t update(int32_t offset, const std::string& newStr);
        void remove(int32_t offset);

        std::string get(int32_t offset) const;
        std::vector<std::string> getAll() const;

        inline size_t getStorageSize() const { return _memoryPool.getTotalSize(); }

    private:
        static void free_storage_func(size_t offset, size_t size, void* pUserData);
        static bool validate_free_range_func(size_t offset, size_t size, void* pUserData);
    };
}
