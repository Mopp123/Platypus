#include "FileUtils.h"
#include <fstream>
#include "platypus/core/Debug.h"


namespace platypus
{
    std::vector<char> load_file(const std::string& filepath)
    {
        std::vector<char> buffer;
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            Debug::log(
                "@load_file "
                "Failed to open file from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return buffer;
        }

        size_t fileSize = (size_t)file.tellg();
        buffer.resize(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
}
