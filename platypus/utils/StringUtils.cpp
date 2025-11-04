#include "StringUtils.hpp"


namespace platypus
{
    void trim_spaces(std::string& target)
    {
        size_t foundPos = target.find(" ");
        while (foundPos != target.npos)
        {
            target.erase(foundPos, 1);
            foundPos = target.find(" ");
        }
    }
}
