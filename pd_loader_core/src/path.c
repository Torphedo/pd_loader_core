#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <shlobj.h>
#include <stdio.h>

#include "path.h"

void get_ms_esper_path(char* string) {
    static char ms_esper_path[MAX_PATH] = {0};
    static uint32_t path_length = 0;

    // Only get the path if it's empty
    if (ms_esper_path[0] == 0) {
        if (SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA, NULL, 0, ms_esper_path) == S_OK) {
            path_length = strlen(ms_esper_path);
            // Remove the "AC" from the end of the path
            memset(&ms_esper_path[path_length - 2], 0, 2);
            // Add "RoamingState\" to the end
            strncat(ms_esper_path, "RoamingState\\", sizeof("RoamingState\\") - 1);
            path_length += sizeof("RoamingState\\") - 1;
        }
    }
    strncpy(string, ms_esper_path, path_length);
}

bool path_has_extension(const char* path, const char* extension) {
    uint32_t pos = strlen(path);
    uint16_t ext_length = strlen(extension);

    // File extension is longer than input string.
    if (ext_length > pos) {
        return false;
    }
    return (strncmp(&path[pos - ext_length], extension, ext_length) == 0);
}

void path_fix_backslashes(char* path) {
    uint16_t pos = strlen(path) - 1; // Subtract 1 so that we don't need to check null terminator
    while (pos > 0) {
        if (path[pos] == '\\') {
            path[pos] = '/';
        }
        pos--;
    }
}

void path_truncate(char* path, uint16_t pos) {
    path[--pos] = 0; // Removes last character to take care of trailing "\\" or "/".
    while(path[pos] != '\\' && path[pos] != '/') {
        path[pos--] = 0;
    }
}

void path_get_filename(const char* path, char* output) {
    uint16_t pos = strlen(path);
    while(path[pos] != '\\' && path[pos] != '/') {
        pos--;
    }
    strcpy(output, &path[pos] + 1);

}

void path_make_physfs_friendly(char* path) {
    // Copy string to a new buffer
    char string_cpy[MAX_PATH] = {0};
    path_fix_backslashes(path);

    // Handle drive letters in the middle of paths like "/Assets/Data/t:/charaselparam"
    char drive_letter = 0;
    if(sscanf(path, "Assets/Data/%c:/%s", &drive_letter, string_cpy) > 1) {
        sprintf(path, "/%c/%s", drive_letter, string_cpy);
        return;
    }

    strncpy(string_cpy, path, MAX_PATH);
    memset(path, 0, MAX_PATH); // Delete input string

    for(uint16_t i = 0; i < MAX_PATH; i++) {
        if (string_cpy[i] == '/') {
            // In case of "//" in the filepath, skip first slash.
            if (string_cpy[i + 1] == '/') {
                continue;
            }
            else if (memcmp(&string_cpy[i], "/../", 4) == 0) {
                path_truncate(path, i); // Remove the last directory that was added to the output string
                i += 2; // Skip over the "/.."
                continue;
            }
            else if (memcmp(&string_cpy[i], "/./", 3) == 0) {
                i++; // Skip over the "/."
                continue;
            }
        }
        // Write the current character to the output path. strncat() is used because skipping over a character
        // and simply writing to the current position in the target string leaves behind null terminators.
        strncat(path, &string_cpy[i], 1);
    }
}
