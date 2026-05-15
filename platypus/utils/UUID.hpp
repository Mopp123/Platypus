#pragma once

#include <cstdint>
#include <set>

#define NULL_UUID 0


typedef uint64_t UUID_t;


namespace platypus
{
    class UUID
    {
    private:
        static unsigned int s_seed;
        static std::set<UUID_t> s_usedIDs;
        static bool s_initialized;

    public:
        static UUID_t generate();
        static bool occupy(UUID_t id);
        static void erase(UUID_t idToErase);

        static UUID_t hash(UUID_t a, UUID_t b);

    private:
        static void init();
    };
}
