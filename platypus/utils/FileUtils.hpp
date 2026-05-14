#pragma once

#include <vector>
#include <string>


namespace platypus
{
    // For reading and writing binary files
    std::vector<char> read_file(const std::string& filepath);
    void write_file(const std::string& filepath, const std::vector<char>& data);

    std::string read_text_file(const std::string& filepath);

    bool validate_file(const std::string& filepath, std::string& error);
}
