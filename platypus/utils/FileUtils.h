#pragma once

#include <vector>
#include <string>


namespace platypus
{
    // Returns file's binary data.
    // Currently used for loading spir-v shaders and text files.
    // NOTE: Pretty insecure!
    std::vector<char> load_file(const std::string& filepath);
}
