#include "dll.h"

#include <common/path.h>
#include <common/logging.h>

#include "process.h"

bool remote_load_library_existing(HANDLE remote, const char* dll_path) {
    // Allocate & write path in remote process
    uint32_t size = strnlen(dll_path, MAX_PATH) + 1;
    char* remote_path = VirtualAllocEx(remote, NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!remote_path) {
        LOG_MSG(error, "Failed to allocate %d bytes for remote filepath \"%s\"\n", size, dll_path);
        CloseHandle(remote);
        return false;
    }

    bool result = WriteProcessMemory(remote, remote_path, dll_path, size, NULL);
    if (!result) {
        LOG_MSG(error, "Failed to write DLL filepath!\n");
        goto exit;
    }

    // Get &LoadLibraryA()
    void* load_library = get_remote_proc_addr("KERNEL32.dll", "LoadLibraryA", remote);

    // Start execution.
    HANDLE remote_thread = CreateRemoteThread(remote, NULL, 0, load_library, remote_path, 0, NULL);
    if (!remote_thread) {
        LOG_MSG(error, "Failed to start remote thread.\n");
        result = false;
        goto exit;
    }

    WaitForSingleObject(remote_thread, INFINITE);

exit:
    VirtualFreeEx(remote, remote_path, 0, MEM_RELEASE);
    CloseHandle(remote);
    return result;
}

bool remote_load_library(uint32_t pid, const char* dll_path) {
	// Try to open the target process with required permissions
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION;
	HANDLE remote = OpenProcess(access, false, pid);
	if (remote == NULL) {
		LOG_MSG(error, "Failed to open target process %d\n", pid);
		return false;
	}

    return remote_load_library_existing(remote, dll_path);
}