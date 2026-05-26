#include "UUID.hpp"
#include <time.h>
#include "platypus/core/Debug.hpp"


namespace platypus
{
    std::mt19937_64 UUID::s_randomEngine;
    std::unordered_map<uint32_t, std::set<UUID_t>> UUID::s_IDPools;
    bool UUID::s_initialized = false;

    // NOTE: Could be done better...
    UUID_t UUID::generate(uint32_t poolID)
    {
        if (!s_initialized)
            init();

        if (s_IDPools.find(poolID) == s_IDPools.end())
            s_IDPools[poolID] = { };

        std::set<UUID_t>& IDPool = s_IDPools[poolID];
        UUID_t id = s_randomEngine();
        PLATYPUS_ASSERT(id != NULL_UUID);

        if (IDPool.find(id) != IDPool.end())
        {
            Debug::log(
                "UUID: " + std::to_string(id) + " already exists in pool: " + std::to_string(poolID) + "! "
                "With current system this should never happen!?!?",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        IDPool.insert(id);

        return id;
    }

    bool UUID::occupy(UUID_t id, uint32_t poolID)
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

        if (s_IDPools.find(poolID) == s_IDPools.end())
        {
            Debug::log(
                "IDPool: " + std::to_string(poolID) + " not found!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        std::set<UUID_t>& IDPool = s_IDPools[poolID];
        if (IDPool.find(id) != IDPool.end())
        {
            Debug::log(
                "UUID: " + std::to_string(id) + " was already in use in pool: " + std::to_string(poolID),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        IDPool.insert(id);
        return true;
    }

    void UUID::erase(UUID_t idToErase, uint32_t poolID)
    {
        if (s_IDPools.find(poolID) == s_IDPools.end())
        {
            Debug::log(
                "IDPool: " + std::to_string(poolID) + " not found!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        std::set<UUID_t>& IDPool = s_IDPools[poolID];
        std::set<UUID_t>::iterator it = IDPool.find(idToErase);
        if (it == IDPool.end())
        {
            Debug::log(
                "UUID: " + std::to_string(idToErase) + " not found from pool: " + std::to_string(poolID),
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

    bool UUID::exists(UUID_t uuid, uint32_t poolID)
    {
        if (s_IDPools.find(poolID) == s_IDPools.end())
            return false;

        const std::set<UUID_t>& poolIDs = s_IDPools[poolID];
        return poolIDs.find(uuid) != poolIDs.end();
    }

    uint32_t UUID::get_free_pool_ID()
    {
        uint32_t poolID = s_IDPools.size();
        if (poolID == 0)
            poolID = 1;

        if (s_IDPools.find(poolID) != s_IDPools.end())
        {
            Debug::log(
                "Pool ID: " + std::to_string(poolID) + " was already taken! "
                "This might have happened because you used non default(0) pool ID "
                "without getting it from this function! Convoluted and awful system, I know:D",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return poolID;
    }

    void UUID::erase_pool_ID(uint32_t poolID)
    {
        if (s_IDPools.find(poolID) == s_IDPools.end())
        {
            Debug::log(
                "No pool ID: " + std::to_string(poolID) + " found!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        s_IDPools.erase(poolID);
    }

    void UUID::init()
    {
        s_randomEngine.seed(time(nullptr));
        s_initialized = true;
    }
}
