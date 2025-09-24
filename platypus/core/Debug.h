#pragma once

#include <string>
#include <cassert>

#define PLATYPUS_ASSERT(arg) assert(arg)

#if defined(__clang__)
    // NOTE: Not sure if works. Not tested!
    #define PLATYPUS_CURRENT_FUNC_NAME __func__
#elif defined(__GNUC__) || defined(__GNUG__)
    #define PLATYPUS_CURRENT_FUNC_NAME __func__
#elif defined(_MSC_VER)
    // NOTE: Not sure if works. Not tested!
    #define PLATYPUS_CURRENT_FUNC_NAME __FUNCTION__
#else
    #define PLATYPUS_CURRENT_FUNC_NAME "<FAILED TO GET FUNC NAME>"
#endif

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
