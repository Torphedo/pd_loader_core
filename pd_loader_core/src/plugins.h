#pragma once
#include <stdbool.h>
#include <stdint.h>

/// Get a handle to another plugin DLL using its filename. Replaces WinAPI's GetModuleHandle() for plugin DLLs.
/// @param filename The filename of the DLL to search for
/// @return A handle for use with plugin_get_proc_address, or NULL on failure
void* plugin_get_module_handle(const char* filename);

/// Get the address of a function in another plugin DLL. Replaces WinAPI's GetProcAddress() for plugin DLLs.
/// @param handle A handle to the plugin obtained from plugin_get_module_handle()
/// @param function_name The name of the function
/// @return Pointer to the requested function, or NULL on failure
void* plugin_get_proc_address(void* handle, const char* function_name);
