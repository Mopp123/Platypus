#pragma once

#include <set>

namespace platypus
{
    struct BufferImpl
    {
        uint32_t id;
        std::set<uint32_t> vaos;
    };
}
