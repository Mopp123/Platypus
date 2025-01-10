#include "Debug.h"
#include <iostream>

#ifdef PLATYPUS_BUILD_WEB
#include <emscripten/console.h>
#endif

// NOTE: Not remembering anymore wtf "LS" stands for here...
#define LS_DEBUG_MESSAGE_TAG__MESSAGE		""
#define LS_DEBUG_MESSAGE_TAG__WARNING		"(Warning)"
#define LS_DEBUG_MESSAGE_TAG__ERROR			"[ERROR]"
#define LS_DEBUG_MESSAGE_TAG__FATAL_ERROR	"<!FATAL ERROR!>"


namespace platypus
{
    void Debug::log(std::string message, MessageType t)
    {
        std::string fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__ERROR) + " Invalid message type: " + std::to_string(t);
        switch (t)
        {
            case platypus::Debug::PLATYPUS_MESSAGE  : fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__MESSAGE) + " " + message; break;
            case platypus::Debug::PLATYPUS_WARNING  : fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__WARNING) +  " " + message; break;
            case platypus::Debug::PLATYPUS_ERROR    : fullMsg = std::string(LS_DEBUG_MESSAGE_TAG__ERROR) +  " " + message; break;
            default:
                break;
        }

    #ifdef PLATYPUS_BUILD_WEB
        emscripten_console_log(fullMsg.c_str());
    #else
        std::cout << fullMsg << std::endl;
    #endif
    }
}
