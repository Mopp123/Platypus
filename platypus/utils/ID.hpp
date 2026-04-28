#pragma once

#include <cstdint>
#include <set>

#define NULL_ID 0


typedef uint32_t ID_t;


namespace platypus
{
    class ID
    {
    private:
        static unsigned int s_seed;
        static std::set<ID_t> s_usedIDs;
        static bool s_initialized;

    public:
        static ID_t generate();
        static bool occupy(ID_t id);
        static void erase(ID_t idToErase);

        static ID_t hash(ID_t a, ID_t b);

    private:
        static void init();
    };
}
