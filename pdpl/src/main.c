#include <stdlib.h>
#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <direct.h>

#include <common/file.h>
#include <common/logging.h>

#include "dll.h"
#include "process.h"

int main() {
    static char core_path[MAX_PATH] = {0};
    // Copy pd_loader_core.dll to a place where it can be read by the game, if it doesn't exist there.
    SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA, NULL, 0, core_path);
    strncat(core_path, "\\Packages\\Microsoft.MSEsper_8wekyb3d8bbwe\\RoamingState\\mods", sizeof(core_path) - 1);
    _mkdir(core_path);
    strncat(core_path, "\\pd_loader_core.dll", sizeof(core_path) - 1);
    if (!file_exists(core_path)) {
        if (!file_exists("pd_loader_core.dll")) {
            LOG_MSG(error, "Couldn't find pd_loader_core.dll. Please place this file in the mods folder or next to the program.\n");
            system("pause");
        } else {
            LOG_MSG(info, "Copying pd_loader_core.dll into the mods folder for first-time setup.\n");
            CopyFile("pd_loader_core.dll", core_path, FALSE);
        }
    }

    // The easy way, but sometimes flagged by antivirus for being a giant command.
    // system("if not exist %LOCALAPPDATA%\\Packages\\Microsoft.MSEsper_8wekyb3d8bbwe\\RoamingState\\mods\\pd_loader_core.dll (cp pd_loader_core.dll %LOCALAPPDATA%\\Packages\\Microsoft.MSEsper_8wekyb3d8bbwe\\RoamingState\\mods\\pd_loader_core.dll)");

    // Kill Phantom Dust if it's already running
    uint32_t process_id = get_pid_by_name("PDUWP.exe");
    if (process_id != 0) {
        // Terminate Phantom Dust
        void* process = OpenProcess(PROCESS_TERMINATE, false, process_id);
        if (!process) {
            LOG_MSG(error, "Unable to open process ID %d for termination.\n", process_id);
            system("pause");
            return EXIT_FAILURE;
        }
        TerminateProcess(process, EXIT_SUCCESS);
        CloseHandle(process);
    }

    // Run Phantom Dust
    system("explorer shell:AppsFolder\\Microsoft.MSEsper_8wekyb3d8bbwe!App");
    enable_debug_privilege(true);

    // Wait for the game to start, so we can get a handle to it
    process_id = 0;
    while (process_id == 0) {
        Sleep(1);
        process_id = get_pid_by_name("PDUWP.exe");
    }

    LOG_MSG(info, "Injecting mods into Phantom Dust...\n");
    remote_load_library(process_id, core_path);

	return EXIT_SUCCESS;
}