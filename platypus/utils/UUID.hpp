#pragma once

#include <cstdint>
#include <set>
#include <unordered_map>
#include <random>

#define NULL_UUID 0


typedef uint64_t UUID_t;


namespace platypus
{
    class UUID
    {
    private:
        static std::mt19937_64 s_randomEngine;
        static std::unordered_map<uint32_t, std::set<UUID_t>> s_IDPools;
        static bool s_initialized;

    public:
        static UUID_t generate(uint32_t poolID = 0);
        static bool occupy(UUID_t id, uint32_t poolID = 0);
        static void erase(UUID_t idToErase, uint32_t poolID = 0);

        static UUID_t hash(UUID_t a, UUID_t b);

    private:
        static void init();
    };
}
