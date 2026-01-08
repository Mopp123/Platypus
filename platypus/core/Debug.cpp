#include "Debug.h"
#include <iostream>

#ifdef PLATYPUS_BUILD_WEB
#include <emscripten/console.h>
#endif

// NOTE: Not remembering anymore wtf "LS" stands for here...
#define LS_DEBUG_MESSAGE_TAG__MESSAGE		""
#define LS_DEBUG_MESSAGE_TAG__WARNING		"(Warning) "
#define LS_DEBUG_MESSAGE_TAG__ERROR			"[ERROR] "
#define LS_DEBUG_MESSAGE_TAG__FATAL_ERROR	"<!FATAL ERROR!> "


namespace platypus
{
    void Debug::log(std::string message, const char* callLocation, MessageType t)
    {
        std::string fullMsg;
        switch (t)
        {
            case MessageType::PLATYPUS_MESSAGE  : fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__MESSAGE); break;
            case MessageType::PLATYPUS_WARNING  : fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__WARNING); break;
            case MessageType::PLATYPUS_ERROR    : fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__ERROR); break;
            default:
                break;
        }
        if (callLocation)
        {
            fullMsg += "@" + std::string(callLocation) + " ";
        }
        fullMsg += message;

    #ifdef PLATYPUS_BUILD_WEB
        emscripten_console_log(fullMsg.c_str());
    #else
        std::cout << fullMsg << std::endl;
    #endif
    }

    void Debug::log(std::string message, MessageType t)
    {
        log(message, nullptr, t);
    }
}
