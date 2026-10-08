#pragma once

#include "Memory.hpp"
#include <string>


namespace platypus
{
    class StringPool
    {
    private:
        struct StoredString
        {
            uint32_t totalSize = 0;
            uint32_t dataSize = 0;
            const void* pData = nullptr;
        };

        DynamicElementSizeMemoryPool _memoryPool;

    public:
        int32_t add(const std::string& str);
        int32_t update(int32_t offset, const std::string& newStr);
        void remove(int32_t offset);
        std::string get(int32_t offset) const;

    private:
        StoredString toStoredString(const std::string& str) const;
    };
}
