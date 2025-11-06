#include "process.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <psapi.h>
#include <tlhelp32.h> // For process snapshots

#include <common/path.h>
#include <common/logging.h>

uint32_t get_pid_by_name(const char* process_name) {
    PROCESSENTRY32 entry = {
        .dwSize = sizeof(PROCESSENTRY32)
    };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (!lstrcmpi(entry.szExeFile, process_name)) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot); // close handle on failure
    return 0;
}

bool enable_debug_privilege(bool bEnable) {
	HANDLE hToken = NULL;
	LUID luid;
	
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		return FALSE;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
		return FALSE;
	}

	TOKEN_PRIVILEGES tokenPriv = {
		.PrivilegeCount = 1,
		.Privileges[0] = {
			.Luid = luid,
			.Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0
		}
	};
	if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
		return FALSE;
	}
	return TRUE;
}

HMODULE* get_remote_module_list(HANDLE h) {
    // Find out how much space we actually need. Hate this API.
    HMODULE dummy = 0;
    DWORD modules_size = 0;
    EnumProcessModules(h, &dummy, sizeof(HMODULE), &modules_size);

    HMODULE* modules = calloc(1, modules_size);
    if (modules == NULL) {
        return NULL;
    }
    EnumProcessModules(h, modules, modules_size, &modules_size);

    return modules;
}

void* get_remote_module(const char* module_name, HANDLE target) {
	// Find out how many modules to expect. This API sucks.
	HMODULE dummy = 0;
	DWORD modules_size = 0;
	EnumProcessModules(target, &dummy, sizeof(HMODULE), &modules_size);

	HMODULE modules[128] = {0};
	EnumProcessModules(target, modules, modules_size, &modules_size);

	char module_path[MAX_PATH] = {0};
	HMODULE output = 0;
	uint64_t module_count = (modules_size / sizeof(HMODULE)) + 1;
	for (uint32_t i = 0; i < module_count; i++) {
		DWORD res = GetModuleFileNameEx(target, modules[i], module_path, sizeof(module_path) - 1);
        if (res == 0 || strlen(module_path) == 0) {
            continue;
        }

		char module_filename[MAX_PATH] = {0};
		path_get_filename(module_path, module_filename);

		// strnicmp() is case-insensitive
		if (strnicmp(module_filename, module_name, MAX_PATH) == 0) {
			output = modules[i];
			break;
		}

		// Wipe the string so when we overwrite it again the null terminator
		// will be correct
		memset(module_path, 0x00, MAX_PATH);
	}

	return output;
}

void* get_remote_proc_addr(const char* module_name, const char* proc_name, HANDLE h) {
	void* local_module = GetModuleHandle(module_name);
	if (local_module == NULL) {
		LOG_MSG(error, "Module %s not found in local process.\n", module_name);
		return NULL;
	}
	void* local_proc = GetProcAddress((void*)local_module, proc_name);
	if (local_proc == NULL) {
		LOG_MSG(error, "Couldn't find %s::%s() locally\n", module_name, proc_name);
		return NULL;
	}
	uintptr_t remote_module = (uintptr_t)get_remote_module(module_name, h);

	// Offset of function in DLL
	uintptr_t proc_offset = ((uintptr_t)local_proc - (uintptr_t)local_module);

	return (void*)((uintptr_t)remote_module + proc_offset);
}

