#include <string.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOMB

#include <Windows.h>

#include <physfs.h>
#include <common/path.h>

#include "filesystem.h"
#include "pd_path.h"

void vfs_recursive_mount(const char* dir, bool appendToPath) {
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
            PHYSFS_mount(full_path, "/", appendToPath);
        }
    }
    PHYSFS_freeList(file_list);

}

void vfs_setup() {
    printf("\n%s: Starting up...\n", vfs_msg);
    PHYSFS_init(NULL);

    // Get game RoamingState path
    char app_path[MAX_PATH] = {0};
    get_roaming_state_path(app_path);

    // Enabling writing to this directory and make the mod / plugin folders if necessary
    PHYSFS_setWriteDir(app_path);
    PHYSFS_mkdir("mods/plugins");

    // Add mod folder to the search path. This MUST come before the archive
    // mounting...because the archives are in the mod folder. It would be
    // ridiculous to mount this after the archives. I would NEVER make that mistake.
    strcat(app_path, "mods");
    if (PHYSFS_mount(app_path, "/", true) == 0) {
        printf("%s: Failed to add %s to the virtual filesystem. (%s)\n", vfs_err, app_path, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        printf("%s: Mounted %s at root.\n", vfs_msg, app_path);
    }

    // Mount all archives
    vfs_recursive_mount("/", true);

    // Mount vanilla game at root as the last resort.
    char pd_path[MAX_PATH] = {0};
    get_pd_path(pd_path);
    PHYSFS_mount(pd_path, "/", true);
    // printf("%s: Mounting vanilla game (%s).\n", vfs_msg, app_path);

    printf("%s: Finished startup.\n", vfs_msg);
}
