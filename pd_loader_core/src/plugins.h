#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void (*plugin_cleanup)(void* dll_handle);
    bool (*PHYSFS_exists)(const char* path);
    void* (*PHYSFS_openRead)(const char* path);
    void* (*PHYSFS_openWrite)(const char* path);
    void* (*PHYSFS_openAppend)(const char* path);
    int64_t (*PHYSFS_readBytes)(void* handle, void* buffer, uint64_t size);
    int64_t (*PHYSFS_writeBytes)(void* handle, const void* buffer, uint64_t size);
    int64_t (*PHYSFS_fileLength)(void* handle);
    int64_t (*PHYSFS_tell)(void* handle);
    int32_t (*PHYSFS_seek)(void* handle, uint64_t pos);
    int (*PHYSFS_close)(void* handle);
    char** (*PHYSFS_enumerateFiles)(const char* directory);
    void (*PHYSFS_freeList)(void* list);
    uint32_t (*plugin_get_count)();
    void** (*plugin_get_handles)();
    void* (*plugin_get_proc_address)(void* plugin_handle, const char* name);
}plugin_api;

typedef int (__stdcall *PLUGINMAIN)(void* dll_handle, const plugin_api* exposed_functions);

void load_plugins();
