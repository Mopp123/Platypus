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
        static std::vector<std::set<UUID_t>> s_IDPools;
        static std::vector<size_t> s_freePoolIndices;
        static bool s_initialized;

    public:
        static UUID_t generate(size_t poolIndex);
        static bool occupy(UUID_t id, size_t poolIndex);
        static void erase(UUID_t idToErase, size_t poolIndex);
        static UUID_t hash(UUID_t a, UUID_t b);
        static bool exists(UUID_t uuid, size_t poolIndex);
        static size_t occupy_pool();
        static void erase_pool(size_t poolIndex);

    private:
        static void init();
        static bool pool_exists(size_t poolIndex);
    };
}
