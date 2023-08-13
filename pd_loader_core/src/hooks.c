#include <stdio.h>

// Allow use of LPCREATEFILE2_EXTENDED_PARAMETERS on MinGW
#ifdef _WIN32_WINNT
    #if(_WIN32_WINNT <= 0x601)
        #undef _WIN32_WINNT
        #define _WIN32_WINNT 0x0602
    #endif // #if(_WIN32_WINNT <= 0x601)
#endif // #ifdef _WIN32_WINNT

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
    bool is_physfs_archive;
}file_handle;

// Stores pairs of PhysicsFS and Windows file handles.
// Enough for the game to open 32,768 files at once. There are roughly 3,800 files in the game.
file_handle handle_list[0x8000] = {0};
uint16_t handle_list_pos = 0;
// Stores which handle pairs are free to be overwritten (by index). When a handle is closed, it's added to this list and the handle pair is set to NULL.
// When a free handle pair is overwritten, its entry in this list is set to 0xFFFF. The list should be initialized to 0xFFFF.
uint16_t* free_list = NULL;
uint16_t free_count = 0;


typedef HANDLE (*CREATE_FILE_2)(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS);
CREATE_FILE_2 original_CreateFile2 = NULL;
CREATE_FILE_2 addr_CreateFile2 = NULL;

typedef int (*READ_FILE)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
READ_FILE addr_ReadFile = NULL;
READ_FILE original_ReadFile = NULL;

HANDLE read_mutex = NULL;

bool lock_filesystem = true;

HANDLE hook_CreateFile2(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams) {
    char path[MAX_PATH] = {0};
    wchar_t wide_path[MAX_PATH] = {0};
    PHYSFS_utf8FromUtf16(lpFileName, path, MAX_PATH);
    path_make_physfs_friendly(path);

    static file_handle handles = {0};

    // Try to open the file with the default function.
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
    // printf(" Opened %s\n", path);
    printf(" Opened %s [from real path %s]\n", path, PHYSFS_getRealDir(path));

    // File exists in PhysicsFS but not in the real filesystem. So we create a blank file of the same size for the
    // Windows handle.
    if (handles.physfs_handle != NULL && handles.win_handle == INVALID_HANDLE_VALUE) {

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
        printf("Opening a fake file for Windows at %s.\n", fake_path);
        handles.win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
        printf("PhysicsFS handle was %p.\n", handles.physfs_handle);

        const char* real_path = PHYSFS_getRealDir(path);
        if (path_has_extension(real_path, ".zip") || path_has_extension(real_path, ".7z") || path_has_extension(real_path, ".rar")) {
            handles.is_physfs_archive = true;
        }
    }

    MH_EnableHook(addr_CreateFile2);

    if (free_count == 0) {
        // Insert new entry in handle list and increment position
        handle_list[handle_list_pos++] = handles;
    }
    else {
        uint16_t index = free_list[--free_count];
        free_list[free_count] = 0xFFFF;
        handle_list[index] = handles;
    }

    if (handles.is_physfs_archive) {
        printf("CreateFile2(): Leaving the function.\n");
    }
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
    uint32_t wait_result = WaitForSingleObject(read_mutex, INFINITE);

    if (wait_result == WAIT_OBJECT_0) {
        // We got ownership of the mutex
        file_handle handles = {0};
        for (uint32_t i = 0; i < handle_list_pos; i++) {
            if (hFile == handle_list[i].win_handle) {
                handles = handle_list[i];
            }
        }

        int result = 0;
        if (handles.physfs_handle != NULL && handles.is_physfs_archive) {
            if (lpOverlapped != NULL) {
                PHYSFS_seek(handles.physfs_handle, (uint64_t) lpOverlapped->Offset | (uint64_t) lpOverlapped->OffsetHigh);
            }
            printf("Using PHYSFS handle.\n");
            result = PHYSFS_readBytes(handles.physfs_handle, lpBuffer, nNumberOfBytesToRead);
            char* buf = malloc(nNumberOfBytesToRead);
            if (buf != NULL) {
                result = original_ReadFile(hFile, buf, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
                free(buf);
            }
        }
        else {
            result = original_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
        }

        if (!ReleaseMutex(read_mutex)) {
            printf("ReadFile(): Failed to release mutex!\n");
        }
        return result;
    }
    else if (wait_result == WAIT_ABANDONED) {
        printf("ReadFile(): Read mutex abandoned. Panic time!\n");
        return 0;
    }
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
    read_mutex = CreateMutex(NULL, FALSE, NULL);

    if (MH_EnableHook(addr_CreateFile2) != MH_OK) {
        printf("Failed to enable hook.\n");
        return false;
    }

    return true;
}
