#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <MinHook.h> // Also includes Windows.h

#include "console.h"
#include "hooks.h"
#include "filesystem.h"
#include "plugins.h"
#include "commands.h"

bool running = true;
int command_exit(int argc, char** argv) {
    running = false;
    return 0;
}

void __stdcall loader_main(void* plugin_handle) {
    hooks_setup_lock_files();
    command_sys_init();
    console_setup(32000);
    SetConsoleTitle("Phantom Dust Plugin Console");

    vfs_setup();
    load_plugins();
    command_register(command_exit, "", "exit");
    command_register(command_exit, "", "quit");

    printf("%s: Unlocking files for read/write...\n\n", vfs_msg);
    hooks_unlock_filesystem();

    while (running) {
        char line[MAX_PATH] = {0};
        printf("> ");
        fgets(line, ARRAYSIZE(line), stdin);
        command_exec(line);
    }
}

int32_t __stdcall DllMain(HINSTANCE dll_handle, uint32_t reason, void* reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Disable DLL notifications for new threads starting up, because we have no need to run special code here.
        DisableThreadLibraryCalls(dll_handle);

        // Start injected code
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)loader_main, dll_handle, 0, NULL);
    }
    return TRUE;
}
