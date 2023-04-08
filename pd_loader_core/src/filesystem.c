#include <string.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOMB

#include <Windows.h>

#include <physfs.h>

#include "filesystem.h"
#include "path.h"
#include "hooks.h"

static const char vfs_msg[] = "[\033[32mVirtual Filesystem\033[0m]";
static const char vfs_err[] = "[\033[31mVirtual Filesystem\033[0m]";

void vfs_setup() {
    printf("\n%s: Starting up...\n", vfs_msg);
    PHYSFS_init(NULL);

    // Get game RoamingState path
    char app_path[MAX_PATH] = { 0 };

    get_ms_esper_path(app_path);

    // Enabling writing to this directory and make the mod / plugin folders if necessary
    PHYSFS_setWriteDir(app_path);
    PHYSFS_mkdir("mods/plugins");

    // Add mod folder to the search path. This MUST come before the archive mounting...because the archives
    // are in the mod folder. What a silly thing to mount this after the archives. I have NEVER made this mistake.
    strcat(app_path, "mods");
    if (PHYSFS_mount(app_path, "/", true) == 0) {
        printf("%s: Failed to add %s to the virtual filesystem. (%s)\n", vfs_err, app_path, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
    else {
        printf("%s: Mounted %s at root.\n", vfs_msg, app_path);
    }

    // Recursive Archive Mounter
    static const char* dir = "/";
    char** file_list = PHYSFS_enumerateFiles(dir);

    for (char** i = file_list; *i != NULL; i++) {
        if (path_has_extension(*i, ".7z") || path_has_extension(*i, ".zip")) {
            char full_path[MAX_PATH] = {0};

            // Get full virtual filesystem path.
            sprintf(full_path, "%s%s", dir, *i);
            printf("%s: Mounted %s at root.\n", vfs_msg, full_path);

            // Real search path + / or \ + filename
            sprintf(full_path, "%s%s%s",PHYSFS_getRealDir(full_path), PHYSFS_getDirSeparator(), *i);

            // Mount to the current virtual directory.
            PHYSFS_mount(full_path, "/", true);
        }
    }
    PHYSFS_freeList(file_list);

    memset(app_path, 0, MAX_PATH); // Clear string so there are no leftovers of the old string
    // Get path to Phantom Dust files by getting location of PDUWP.exe and truncating the filename
    GetModuleFileNameA(NULL, app_path, MAX_PATH);
    path_truncate(app_path, MAX_PATH);
    app_path[strlen(app_path) - 1] = 0; // Cut off trailing backslash

    path_fix_backslashes(app_path);
    // Mount vanilla game at root as the last resort.
    PHYSFS_mount(app_path, "/", true);
    // printf("%s: Mounting vanilla game (%s).\n", vfs_msg, app_path);

    printf("%s: Finished startup.\n", vfs_msg);
    printf("%s: Unlocking files for read/write...\n\n", vfs_msg);
}
