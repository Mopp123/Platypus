#include "StringUtils.hpp"
#include "platypus/core/Debug.hpp"
#include <utf8.h>


namespace platypus { namespace util { namespace str {

    void trim_spaces(std::string& target)
    {
        size_t foundPos = target.find(" ");
        while (foundPos != target.npos)
        {
            target.erase(foundPos, 1);
            foundPos = target.find(" ");
        }
    }

    void append_utf8(uint32_t codepoint, std::string& target)
    {
        utf8::append(codepoint, target);
    }

    void pop_back_utf8(std::string& target)
    {
        if (target.empty())
            return;

        std::string::iterator itEnd = target.end();
        std::string::iterator it = itEnd;
        utf8::prior(it, target.begin());

        size_t lastCharSize = (size_t)(itEnd - it);
        target.erase(it, it + lastCharSize);
    }

    size_t length_utf8(const std::string& str)
    {
        size_t length = 0;
        try
        {
            length = utf8::distance(str.begin(), str.end());
        }
        catch (const utf8::exception& e)
        {
            std::string exceptionStr(e.what());
            Debug::log(
                "Failed to find utf8 string length for string " + str + " "
                "utf8::exception: " + exceptionStr,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return length;
    }

    uint32_t get_first_codepoint(const char* pStr, size_t size)
    {
        utf8::iterator it(pStr, pStr, pStr + size);
        return (uint32_t)*it;
    }

    void iterate_utf8(
        const std::string& str,
        void* pUserData,
        void(*pFunc)(uint32_t, void*)
    )
    {
        const size_t size = str.size();
        char* pData = (char*)str.data();
        utf8::iterator<char*> it(pData, pData, pData + size);
        utf8::iterator<char*> endIt(pData + size, pData, pData + size);

        while (it != endIt)
        {
            uint32_t casted = (uint32_t)*it;
            (*pFunc)(casted, pUserData);
            ++it;
        }
    }
}}}
