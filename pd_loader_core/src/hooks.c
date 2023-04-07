#include <stdio.h>

// Allow use of LPCREATEFILE2_EXTENDED_PARAMETERS on MinGW
#define _WIN32_WINNT 0x0603

#include <Windows.h>

#include <fileapi.h>
#include <MinHook.h>
#include <physfs.h>

#include "hooks.h"
#include "path.h"

// These are both void*
typedef struct {
    PHYSFS_File* physfs_handle;
    HANDLE win_handle;
}file_handle;

typedef HANDLE (*CREATE_FILE_2)(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS);
CREATE_FILE_2 original_CreateFile2 = NULL;
CREATE_FILE_2 addr_CreateFile2 = NULL;

typedef int (*READ_FILE)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
READ_FILE addr_ReadFile = NULL;
READ_FILE original_ReadFile = NULL;

bool lock_filesystem = true;

HANDLE hook_CreateFile2(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams) {
    // During startup, it seems that this function is sometimes called asynchronously. This value is shared across
    // each of these running instances, so while the function is running it will force other instances of the call to wait.
    static bool lock_async_calls = false;
    while (lock_async_calls) {
        Sleep(1);
    }
    lock_async_calls = true;

    char path[MAX_PATH] = {0};
    wchar_t wide_path[MAX_PATH] = {0};
    PHYSFS_utf8FromUtf16(lpFileName, path, MAX_PATH);
    path_make_physfs_friendly(path);

    static file_handle handles = {0};

    if (PHYSFS_exists(path)) {
        // Get the real path of the file and place it in a wide string.
        swprintf(wide_path, MAX_PATH, L"%hs/%hs", PHYSFS_getRealDir(path), path);
        handles.win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
    }
    else {
        handles.win_handle = original_CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
    }

    MH_DisableHook(addr_CreateFile2);
    printf("CreateFile2() ");
    switch(dwDesiredAccess) {
        case GENERIC_READ:
            printf("[\033[32mGENERIC_READ\033[0m]:");
            handles.physfs_handle = PHYSFS_openRead(path);
            break;
        case GENERIC_WRITE:
            printf("[\033[31mGENERIC_WRITE\033[0m]:");
            handles.physfs_handle = PHYSFS_openWrite(path);
            break;
        case (GENERIC_READ | GENERIC_WRITE):
            printf("[\033[33mGENERIC_READ | GENERIC_WRITE\033[0m]:");
            handles.physfs_handle = PHYSFS_openWrite(path);
            break;
        default:
            printf("[UNKNOWN]:");
    }
    printf(" Opened %s\n", path);

    if (PHYSFS_exists(path) && handles.win_handle == INVALID_HANDLE_VALUE) {
        printf("Creating a fake file for Windows handle.\n");
        system("pause");
        static char fake_path[MAX_PATH] = {0};
        get_ms_esper_path(fake_path);
        {
            static char filename[MAX_PATH] = {0}; // Temporary buffer to store filename
            path_get_filename(path, filename);
            sprintf(fake_path, "%sfake\\%s", fake_path, filename);
        }

        // Create a new file with the appropriate size in the /fake/ folder.
        FILE* fake_file = fopen(fake_path, "wb");
        fseek(fake_file, PHYSFS_fileLength(handles.physfs_handle), SEEK_SET);
        fwrite("\0", sizeof("\0"), 1, fake_file);
        fclose(fake_file);

        PHYSFS_utf8ToUtf16(fake_path, wide_path, MAX_PATH);
        handles.win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
        printf("PhysicsFS handle was %p.\n", handles.physfs_handle);
    }

    MH_EnableHook(addr_CreateFile2);

    lock_async_calls = false; // Allow waiting call to run.
    return handles.win_handle;
}

// Stalls until filesystem access is unlocked, then tries to call the original function (which now redirects to hook_CreateFile2()).
HANDLE hook_CreateFile2_stall(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams) {
    while(lock_filesystem) {
        // printf("CreateFile2(): Waiting for virtual filesystem to start...\n");
        Sleep(50);
    }
    return addr_CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
}

// Unused for now.
int hook_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    static bool lock_async_calls = false;
    while (lock_async_calls) {
        Sleep(1);
    }
    lock_async_calls = true;

    int result = original_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

    lock_async_calls = false; // Allow waiting call to run.
    return result;
}

void hooks_unlock_filesystem() {
    if (MH_RemoveHook(addr_CreateFile2) != MH_OK) {
        printf("Failed to remove stall hook.\n");
    }
    if (MH_CreateHook(addr_CreateFile2, &hook_CreateFile2, (void**)&original_CreateFile2) != MH_OK) {
        printf("Failed to create real hook.\n");
    }

    MH_EnableHook(MH_ALL_HOOKS);
    lock_filesystem = false;
}

void* get_procedure_address(const wchar_t* module_name, const char* proc_name) {
    HMODULE hModule = GetModuleHandleW(module_name);
    if (hModule == NULL) {
        return NULL;
    }
    return GetProcAddress(hModule, proc_name);
}

bool hooks_setup_lock_files() {
    MH_Initialize();

    addr_CreateFile2 = get_procedure_address(L"KERNELBASE.dll", "CreateFile2");
    addr_ReadFile = get_procedure_address(L"KERNELBASE.dll", "ReadFile");

    // This "stall" function silently waits around until hooks_unlock_filesystem() is called. Then it's replaced
    // with the real hooked function and the game starts receiving file handles. This lock ensures that plugins
    // have enough time to initialize themselves or create hooks before the game actually starts running.
    if (MH_CreateHook(addr_CreateFile2, &hook_CreateFile2_stall, (void**)&original_CreateFile2) != MH_OK) {
        printf("Failed to create hook.\n");
        return false;
    }

    if (MH_CreateHook(addr_ReadFile, &hook_ReadFile, (void**)&original_ReadFile) != MH_OK) {
        printf("Failed to create hook.\n");
        return false;
    }

    if (MH_EnableHook(addr_CreateFile2) != MH_OK) {
        printf("Failed to enable hook.\n");
        return false;
    }

    return true;
}
