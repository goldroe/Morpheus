#include "log.h"
#include <windows.h>
#include "platform_win32.h"

#include <stdint.h>

inline uint32_t safe_truncate_int32(int64_t value) {
    assert(value <= 0xFFFFFFFF);

    return (uint32_t)value;
}

Platform_File platform_read_file(const char *file_path) {
    Platform_File result{};
    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size{};
        if (GetFileSizeEx(file_handle, &file_size)) {
            uint32_t file_size_32 = safe_truncate_int32(file_size.QuadPart);
            result.contents = VirtualAlloc(NULL, file_size_32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            result.contents_size = file_size_32;
            DWORD bytes_read = 0;
            if (ReadFile(file_handle, result.contents, file_size_32, &bytes_read, NULL) && bytes_read == file_size_32) {
                
            } else {
                ALERT("Reading file %s, size %d", file_path, file_size_32);
                // debug_platform_free_file_memory(result.contents);
            }
        } else {
            ALERT("Getting size of file %s", file_path);
        }
        
        CloseHandle(file_handle);
    } else {
        ALERT("Could not open file %s to read", file_path);
    }

    return result;
}


bool platform_write_file(char *file_path, uint32_t contents_size, void *contents) {
    bool result = true;

    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        DWORD bytes_written = 0;
        if (WriteFile(file_handle, contents, contents_size, &bytes_written, NULL) && bytes_written == contents_size) {
            ALERT("Writing to file %s, memory %p, size %d", file_path, contents, contents_size);
            result = false;
        }
        
        CloseHandle(file_handle);
    }
    else {
        ALERT("Could not open file %s to read", file_path);
        result = false;
    }

    return result;
}

