#pragma once
#include "plugins.h"

/// @brief Load DLL from a VFS path
void* vfs_load_dll(const char* filename);

// Load all DLLs and EXEs found in the "plugins" folder of the VFS
void load_plugins();

// The plugin handle received by plugins' DllMain does not get the correct handle,
// I may change this later to allow people to unload their DLL.
void plugin_cleanup(void* plugin_handle);
