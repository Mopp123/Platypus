#pragma once

#include <string>
#include <cassert>

#define PLATYPUS_ASSERT(arg) assert(arg)

namespace platypus
{
    class Debug
    {
    public:
        enum MessageType
        {
            PLATYPUS_MESSAGE = 0x0,
            PLATYPUS_WARNING,
            PLATYPUS_ERROR
        };

        static void log(std::string message, MessageType t = PLATYPUS_MESSAGE);
    };
}
