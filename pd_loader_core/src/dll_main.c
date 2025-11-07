#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <MinHook.h> // Also includes Windows.h

#include "console.h"
#include "hooks.h"
#include "filesystem.h"
#include "plugins.h"
#include "commands.h"

void __stdcall loader_main(void* plugin_handle) {
    hooks_setup_lock_files();
    console_setup(32000);
    SetConsoleTitle("Phantom Dust Plugin Console");

    vfs_setup();
    load_plugins();
    command_sys_init();

    exec_command("pd_loader_core!hello \"test\" ");

    printf("%s: Unlocking files for read/write...\n\n", vfs_msg);
    hooks_unlock_filesystem();

    // This loop is just here to keep the virtual filesystem and such alive.
    while (true) {
        Sleep(100);
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
