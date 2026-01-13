#include "ID.hpp"
#include <time.h>
#include <cstdlib>
#include <cstring>
#include "platypus/core/Debug.hpp"


namespace platypus
{
    std::set<ID_t> ID::s_usedIDs;
    bool ID::s_initialized = false;

    // NOTE: Could be done better...
    ID_t ID::generate()
    {
        if (!s_initialized)
        {
            std::srand((unsigned int)time(nullptr));
            s_initialized = true;
        }

        ID_t id = 0;
        while ((s_usedIDs.find(id) != s_usedIDs.end()) || id == NULL_ID)
        {
            // randomize each byte individually..
            for (size_t i = 0; i < sizeof(ID_t); ++i)
            {
                uint8_t bVal = (uint8_t)(std::rand() % 255);
                memset(((uint8_t*)&id) + i, bVal, 1);
            }
        }
        s_usedIDs.insert(id);

        return id;
    }

    void ID::erase(ID_t idToErase)
    {
        std::set<ID_t>::iterator it = s_usedIDs.find(idToErase);
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

    // Not sure if this might fail in some cases... use at your own risk...
    // https://stackoverflow.com/questions/919612/mapping-two-integers-to-one-in-a-unique-and-deterministic-way
    ID_t ID::hash(ID_t a, ID_t b)
    {
        uint32_t A = (uint32_t)(a >= 0 ? 2 * (int32_t)a : -2 * (int32_t)a - 1);
        uint32_t B = (uint32_t)(b >= 0 ? 2 * (int32_t)b : -2 * (int32_t)b - 1);
        uint32_t C = (int32_t)((A >= B ? A * A + A + B : A + B * B) / 2);
        ID_t id = (a < 0 && b < 0) || (a >= 0 && b >= 0) ? C : -C - 1;
        if (s_usedIDs.find(id) != s_usedIDs.end())
        {
            Debug::log(
                "@ID::hash "
                "ID: " + std::to_string(id) + " already exists!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        return id;
    }
}
