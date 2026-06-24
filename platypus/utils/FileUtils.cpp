#include "FileUtils.hpp"

#include <fstream>
#include <stdio.h>
#include <cstring>
#include <filesystem>

#include "platypus/core/Debug.hpp"


namespace platypus
{
    std::vector<char> read_file(const std::string& filepath)
    {
        std::vector<char> buffer;
        try
        {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                Debug::log(
                    "Failed to open file: " + filepath,
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return buffer;
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            buffer.resize(fileSize);

            file.seekg(0, std::ios::beg);
            file.read(buffer.data(), fileSize);
            file.close();
        }
        catch (const std::ifstream::failure& e)
        {
            Debug::log(
                "Failed read file: " + filepath + " "
                "(std::ifstream::failure) " + std::string(e.what()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return buffer;
        }
        catch (const std::exception& e)
        {
            Debug::log(
                "Failed read file: " + filepath + " "
                "(std::exception) " + std::string(e.what()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return buffer;
        }

        return buffer;
    }

    void write_file(const std::string& filepath, const std::vector<char>& data)
    {
        try
        {
            std::ofstream file(filepath, std::ios::binary);
            if (!file.is_open())
            {
                Debug::log(
                    "Failed to open file: " + filepath + " for writing",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }

            file.write(data.data(), data.size());
            file.close();
        }
        catch (const std::ifstream::failure& e)
        {
            Debug::log(
                "Failed write file: " + filepath + " "
                "(std::ifstream::failure) " + std::string(e.what()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
        }
        catch (const std::exception& e)
        {
            Debug::log(
                "Failed write file: " + filepath + " "
                "(std::exception) " + std::string(e.what()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
        }
    }


    std::string read_text_file(const std::string& filepath)
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


    std::string get_relative_path(const std::string& absolutePath)
    {
        const std::string currentPath = std::filesystem::current_path().string();
        size_t p = absolutePath.find(currentPath);
        if (absolutePath.size() < currentPath.size())
        {
            Debug::log(
                "Absolute path: " + absolutePath + " length(" + std::to_string(absolutePath.size()) + ") "
                "was shorter than current path: " + currentPath + " length(" + std::to_string(currentPath.size()) + ") "
                "This func requires absolute path be inside the current path!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return "";
        }

        if (p == std::string::npos)
        {
            Debug::log(
                "Current path: " + currentPath + " not found from absolute path: " + absolutePath + ". "
                "Make sure you entered the absolute path correctly!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return "";
        }

        std::string result = absolutePath;
        // erase currentPath + 1 since need to get rid of the first '/'
        return result.erase(p, currentPath.size() + 1);
    }
}
