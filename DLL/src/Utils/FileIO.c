#include "pch.h"
#include "Utils/FileIO.h"

#include <sys/stat.h>

i64 DROP_GetFileTimestamp(const char* fileName)
{
    ASSERT_MSG(fileName, "File path is null.");

    struct stat fileStat = {0};
    stat(fileName, &fileStat);
    return fileStat.st_mtime;
}

bool DROP_FileExists(const char* fileName)
{
    ASSERT_MSG(fileName, "File path is null.");

    FILE* file = fopen(fileName, "r");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s", fileName);
        return false;
    }

    fclose(file);
    return true;
}

u64 DROP_GetFileSize(const char* fileName)
{
    ASSERT_MSG(fileName, "File path is null.");

    FILE* file = fopen(fileName, "rb");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s", fileName);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    fclose(file);
    return size;
}

char* DROP_ReadFile(const char* fileName, u64* pSize, ArenaAllocator* pArena)
{
    ASSERT_MSG(fileName, "File path is null.");
    ASSERT_MSG(pSize, "Size is null.");

    *pSize = 0;

    FILE* file = fopen(fileName, "rb");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s", fileName);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *pSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*) DROP_Allocate(pArena, *pSize + 1);
    if (!buffer)
    {
        LOG_ERROR("Failed to allocate memory for file: %s", fileName);
        return NULL;
    }

    fread(buffer, *pSize, 1, file);
    buffer[*pSize] = '\0'; // Null terminate the buffer to make it a string.

    fclose(file);
    return buffer;
}

bool DROP_WriteFile(const char* fileName, const char* buffer, u64 size)
{
    ASSERT_MSG(fileName, "File path is null.");
    ASSERT_MSG(buffer, "Buffer is null.");
    ASSERT_MSG(size > 0, "Size must be greater than 0.");

    FILE* file = fopen(fileName, "wb+");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s", fileName);
        return false;
    }

    fwrite(buffer, size, 1, file);
    fclose(file);
    return true;
}

bool DROP_CopyFile(const char* source, const char* destination, ArenaAllocator* pArena)
{
    FILE* file = fopen(source, "rb");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s", source);
        return false;
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*) DROP_Allocate(pArena, size);
    fread(buffer, 1, size, file);
    fclose(file);

    if (!DROP_WriteFile(destination, buffer, size))
    {
        LOG_ERROR("Failed to copy file: %s", source);
        return false;
    }

    return true;
}