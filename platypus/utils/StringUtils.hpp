#pragma once

#include <string>
#include <cstdint>


namespace platypus { namespace util { namespace str {

    // NOTE: WARNING: Works only for strings containing 1 byte characters!!!
    void trim_spaces(std::string& target);

    void append_utf8(uint32_t codepoint, std::string& target);
    void pop_back_utf8(std::string& target);
    uint32_t get_first_codepoint(const char* pStr, size_t size);

    // Calls func that gets utf8 codepoint and void* (for user data) for each character in the string
    void iterate_utf8(
        const std::string& str,
        void* pUserData,
        void(*pFunc)(uint32_t, void*)
    );
}}}
