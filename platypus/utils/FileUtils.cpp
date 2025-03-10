#include "FileUtils.h"
#include <fstream>
#include <stdio.h>
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

    std::string load_text_file(const std::string& filepath)
    {
        FILE* pFile = pFile = fopen(filepath.c_str(), "rb");
        if (!pFile)
        {
            Debug::log(
                "@load_text_file "
                "Failed to open file from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return "";
        }

        fseek(pFile, 0, SEEK_END);
        long fileLength = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);

        size_t bufferSize = fileLength + 1;
        char* pBuffer = new char[bufferSize];
        memset(pBuffer, 0, sizeof(char) * bufferSize);

        size_t readBytes = 0;
        readBytes = fread(pBuffer, sizeof(char), fileLength, pFile);

        if (readBytes != fileLength)
        {
            Debug::log(
                "@load_text_file "
                "Error while reading file from: " + filepath + " "
                "(read byte count: " + std::to_string(readBytes) + " "
                "different from file size: " + std::to_string(fileLength) + ")",
                Debug::MessageType::PLATYPUS_ERROR
            );
        }
        std::string sourceStr(pBuffer, fileLength-1);
        delete[] pBuffer;
        return sourceStr;
    }
}
