#include "FileUtils.hpp"

#include <fstream>
#include <stdio.h>
#include <cstring>
#include <filesystem>

#include "platypus/core/Debug.hpp"


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
        FILE* pFile = fopen(filepath.c_str(), "rb");
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


    bool validate_file(const std::string& filepath, std::string& error)
    {
        if (filepath.empty())
        {
            error = "Filepath was empty";
            return false;
        }
        else
        {
            try
            {
                if (!std::filesystem::exists(std::filesystem::path(filepath)))
                {
                    error = "File: " + filepath + " doesn't exist";
                    return false;
                }
                else if (std::filesystem::status(filepath).type() != std::filesystem::file_type::regular)
                {
                    error = "File: " + filepath + " wasn't regular file";
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                error = "Filesystem exception: " + std::string(e.what());
                return false;
            }
        }
        return true;
    }
}
