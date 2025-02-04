#include "ID.h"
#include <time.h>
#include <cstdlib>
#include <cstring>
#include "platypus/core/Debug.h"


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
}
