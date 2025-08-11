#pragma once

i64  DROP_GetFileTimestamp(const char* fileName);
bool DROP_FileExists(const char* fileName);
u64  DROP_GetFileSize(const char* fileName);
// Reads a file into a buffer. This function will use the memory in the ArenaAllocator.
// So the caller doesn't need to free the memory. Because the memory will be freed when the ArenaAllocator is freed.
char* DROP_ReadFile(const char* fileName, u64* pSize, ArenaAllocator* pArena);
// Writes a buffer to disk. The file will be created if it doesn't exist.
bool DROP_WriteFile(const char* fileName, const char* buffer, u64 size);
// Copy file to destination path. This function will use the memory in the ArenaAllocator.
bool DROP_CopyFile(const char* source, const char* destination, ArenaAllocator* pArena);
