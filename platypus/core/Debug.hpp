#pragma once

#include <string>
#include <cassert>

#define PLATYPUS_ASSERT(arg) assert(arg)

// NOTE: Not tested but heard that with clang this should be same as __GNUC__ and __GNUG__?
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
    #define PLATYPUS_CURRENT_FUNC_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    // NOTE: Not sure if works. Not tested!
    #define PLATYPUS_CURRENT_FUNC_NAME __FUNCSIG__
#else
    #define PLATYPUS_CURRENT_FUNC_NAME "<FAILED TO GET FUNC NAME>"
#endif


namespace platypus
{
    class Debug
    {
    public:
        enum class MessageType
        {
            PLATYPUS_MESSAGE,
            PLATYPUS_WARNING,
            PLATYPUS_ERROR
        };

        static void log(
            std::string message,
            const char* callLocation,
            MessageType t = MessageType::PLATYPUS_MESSAGE
        );
        static void log(
            std::string message,
            MessageType t = MessageType::PLATYPUS_MESSAGE
        );
    };
}
