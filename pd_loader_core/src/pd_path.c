#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <shlobj.h>
#include <stdio.h>

#include <common/path.h>
#include "pd_path.h"

void get_roaming_state_path(char* string) {
    static char ms_esper_path[MAX_PATH] = {0};
    static u32 path_length = 0;

    // Only get the path if it's empty
    // (since it's thread-local, we are basically caching it)
    if (ms_esper_path[0] == 0) {
        if (SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA, NULL, 0, ms_esper_path) == S_OK) {
            path_length = strlen(ms_esper_path);
            // Remove the "AC" from the end of the path
            memset(&ms_esper_path[path_length - 2], 0, 2);
            // Add "RoamingState\" to the end
            strncat(ms_esper_path, "RoamingState\\", sizeof("RoamingState\\"));
            path_length += sizeof("RoamingState\\") - 1;
        }
    }
    strncpy(string, ms_esper_path, path_length);
}

void get_fake_file_path(char* string) {
    get_roaming_state_path(string);
    strcat(string, "fake");
}

void get_pd_path(char* out) {
    // Get path to Phantom Dust files by getting location of PDUWP.exe and truncating the filename
    GetModuleFileNameA(NULL, out, MAX_PATH);
    path_truncate(out, MAX_PATH);
    out[strlen(out) - 1] = 0; // Cut off trailing backslash

    path_fix_backslashes(out);
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
