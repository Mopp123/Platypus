#pragma once

#include <vector>
#include <string>


namespace platypus
{
    // Returns file's binary data.
    // Currently used for loading spir-v shaders and text files.
    // NOTE: Pretty insecure!
    // TODO:
    //  *Rename to read file
    //  *Add func for write file
    std::vector<char> load_file(const std::string& filepath);
    std::string load_text_file(const std::string& filepath);

    bool validate_file(const std::string& filepath, std::string& error);
}
