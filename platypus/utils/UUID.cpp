#include "UUID.hpp"
#include <time.h>
#include "platypus/core/Debug.hpp"


namespace platypus
{
    std::mt19937_64 UUID::s_randomEngine;
    std::vector<std::set<UUID_t>> UUID::s_IDPools;
    std::vector<size_t> UUID::s_freePoolIndices;
    bool UUID::s_initialized = false;

    // NOTE: Could be done better...
    UUID_t UUID::generate(size_t poolIndex)
    {
        if (!s_initialized)
            init();

        if (!pool_exists(poolIndex))
        {
            Debug::log(
                "No pool index " + std::to_string(poolIndex) + " exists!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_UUID;
        }

        std::set<UUID_t>& IDPool = s_IDPools[poolIndex];
        UUID_t id = s_randomEngine();
        PLATYPUS_ASSERT(id != NULL_UUID);
        if (IDPool.find(id) != IDPool.end())
        {
            Debug::log(
                "UUID: " + std::to_string(id) + " already exists in pool: " + std::to_string(poolIndex) + "! "
                "With current system this should never happen!?!?",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        IDPool.insert(id);
        return id;
    }

    bool UUID::occupy(UUID_t id, size_t poolIndex)
    {
        if (id == NULL_UUID)
        {
            Debug::log(
                "Can't occupy NULL_UUID explicitly!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        if (!s_initialized)
            init();

        if (!pool_exists(poolIndex))
        {
            Debug::log(
                "No pool index " + std::to_string(poolIndex) + " exists!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        std::set<UUID_t>& IDPool = s_IDPools[poolIndex];
        if (IDPool.find(id) != IDPool.end())
        {
            Debug::log(
                "UUID: " + std::to_string(id) + " was already in use in pool: " + std::to_string(poolIndex),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        IDPool.insert(id);
        return true;
    }

    void UUID::erase(UUID_t idToErase, size_t poolIndex)
    {
        if (!pool_exists(poolIndex))
        {
            Debug::log(
                "No pool index " + std::to_string(poolIndex) + " exists!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        std::set<UUID_t>& IDPool = s_IDPools[poolIndex];
        std::set<UUID_t>::iterator it = IDPool.find(idToErase);
        if (it == IDPool.end())
        {
            Debug::log(
                "UUID: " + std::to_string(idToErase) + " not found from pool: " + std::to_string(poolIndex),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        else
        {
            IDPool.erase(it);
        }
    }

    // NOTE: After upgrading to using uint64_t instead of uint32_t, this might be fucked?
    // Not sure if this might fail in some cases... use at your own risk...
    // https://stackoverflow.com/questions/919612/mapping-two-integers-to-one-in-a-unique-and-deterministic-way
    UUID_t UUID::hash(UUID_t a, UUID_t b)
    {
        // TODO: Make sure this id doesn't exist already in any pool?
        uint32_t A = (uint32_t)(a >= 0 ? 2 * (int32_t)a : -2 * (int32_t)a - 1);
        uint32_t B = (uint32_t)(b >= 0 ? 2 * (int32_t)b : -2 * (int32_t)b - 1);
        uint32_t C = (int32_t)((A >= B ? A * A + A + B : A + B * B) / 2);
        UUID_t id = (a < 0 && b < 0) || (a >= 0 && b >= 0) ? C : -C - 1;
        return id;
    }

    bool UUID::exists(UUID_t uuid, size_t poolIndex)
    {
        if (!pool_exists(poolIndex))
            return false;

        const std::set<UUID_t>& poolIDs = s_IDPools[poolIndex];
        return poolIDs.find(uuid) != poolIDs.end();
    }

    size_t UUID::occupy_pool()
    {
        size_t poolIndex = 0;
        if (!s_freePoolIndices.empty())
        {
            poolIndex = s_freePoolIndices.back();
            s_freePoolIndices.pop_back();
            // This should be unnecessary since freed pool should be already empty...
            s_IDPools[poolIndex] = { };
        }
        else
        {
            poolIndex = s_IDPools.size();
            s_IDPools.push_back({});
        }
        return poolIndex;
    }

    void UUID::erase_pool(size_t poolIndex)
    {
        if (!pool_exists(poolIndex))
        {
            Debug::log(
                "No pool index " + std::to_string(poolIndex) + " exists!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        if (poolIndex == s_IDPools.size() - 1)
        {
            s_IDPools.pop_back();
        }
        else
        {
            s_IDPools[poolIndex].clear();
            s_freePoolIndices.push_back(poolIndex);
        }
    }

    void UUID::init()
    {
        s_randomEngine.seed(time(nullptr));
        s_initialized = true;
    }

    bool UUID::pool_exists(size_t poolIndex)
    {
        for (size_t freePoolIndex : s_freePoolIndices)
        {
            if (poolIndex == freePoolIndex)
                return false;
        }
        return poolIndex < s_IDPools.size();
    }
}
