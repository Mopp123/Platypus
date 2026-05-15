#include "UUID.hpp"
#include <time.h>
#include <cstdlib>
#include <cstring>
#include "platypus/core/Debug.hpp"


namespace platypus
{
    unsigned int UUID::s_seed = 4276;
    std::set<UUID_t> UUID::s_usedIDs;
    bool UUID::s_initialized = false;

    void UUID::init()
    {
        // Not sure should this be more deterministic or not...
        //std::srand(s_seed);
        std::srand(static_cast<unsigned int>(time(nullptr)));
        s_initialized = true;
    }

    // NOTE: Could be done better...
    UUID_t UUID::generate()
    {
        if (!s_initialized)
            init();

        UUID_t id = 0;
        while (true)
        {
            // randomize each byte individually..
            for (size_t i = 0; i < sizeof(UUID_t); ++i)
            {
                uint8_t bVal = (uint8_t)(std::rand() % 255);
                memset(((uint8_t*)&id) + i, bVal, 1);
            }
            if ((s_usedIDs.find(id) != s_usedIDs.end()))
            {
                Debug::log(
                    "ID: " + std::to_string(id) + " already exists! "
                    "Attempting to generate again...",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_WARNING
                );
            }
            else if (id == NULL_UUID)
            {
                Debug::log(
                    "Generated ID was NULL_UUID!"
                    "Attempting to generate again...",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_WARNING
                );
            }
            else
            {
                break;
            }
        }
        s_usedIDs.insert(id);

        return id;
    }

    bool UUID::occupy(UUID_t id)
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

        if (s_usedIDs.find(id) != s_usedIDs.end())
        {
            Debug::log(
                "ID: " + std::to_string(id) + " was already in use!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        s_usedIDs.insert(id);
        return true;
    }

    void UUID::erase(UUID_t idToErase)
    {
        std::set<UUID_t>::iterator it = s_usedIDs.find(idToErase);
        if (it == s_usedIDs.end())
        {
            Debug::log(
                "@ID::erase ID: " + std::to_string(idToErase) + " wasn't found from used IDs",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        else
        {
            s_usedIDs.erase(it);
        }
    }

    // NOTE: After upgrading to using uint64_t instead of uint32_t, this might be fucked?
    // Not sure if this might fail in some cases... use at your own risk...
    // https://stackoverflow.com/questions/919612/mapping-two-integers-to-one-in-a-unique-and-deterministic-way
    UUID_t UUID::hash(UUID_t a, UUID_t b)
    {
        uint32_t A = (uint32_t)(a >= 0 ? 2 * (int32_t)a : -2 * (int32_t)a - 1);
        uint32_t B = (uint32_t)(b >= 0 ? 2 * (int32_t)b : -2 * (int32_t)b - 1);
        uint32_t C = (int32_t)((A >= B ? A * A + A + B : A + B * B) / 2);
        UUID_t id = (a < 0 && b < 0) || (a >= 0 && b >= 0) ? C : -C - 1;
        if (s_usedIDs.find(id) != s_usedIDs.end())
        {
            Debug::log(
                "@ID::hash "
                "UUID: " + std::to_string(id) + " already exists!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_UUID;
        }
        return id;
    }
}
