#include <stdbool.h>
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/// Extremely simple DLL injector using CreateRemoteThread() to call
/// LoadLibraryA([dll_path]) in the remote process.
/// @param pid ID of the target process
/// @param dll_path Path of the DLL to load
/// @return
bool remote_load_library(uint32_t pid, const char* dll_path);

/// Extremely simple DLL injector using CreateRemoteThread() to call
/// LoadLibraryA([dll_path]) in the remote process.
/// @param h A process handle with at least PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
/// @param dll_path Path of the DLL to load
bool remote_load_library_existing(HANDLE h, const char* dll_path);