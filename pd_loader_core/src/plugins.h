#pragma once
#include <stdbool.h>
#include <stdint.h>

void* plugin_get_module_handle(const char* filename);
void* plugin_get_proc_address(void* handle, const char* function_name);
void* vfs_load_dll(char* filename);
void load_plugins();

// The plugin handle received by plugins' DllMain does not get the correct handle,
// I may change this later to allow people to unload their DLL.
void plugin_cleanup(void* plugin_handle);
