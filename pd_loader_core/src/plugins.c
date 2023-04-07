#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOMB

#include <Windows.h>

#include <physfs.h>
#include <MemoryModule.h>

#include "path.h"
#include "plugins.h"

static const char loader_msg[] = "[\033[32mPlugin Loader\033[0m]";
static const char loader_warn[] = "[\033[33mPlugin Loader\033[0m]";

HMEMORYMODULE* plugin_handles = NULL;
uint32_t plugin_count = 0;

uint32_t plugin_get_count() {
    return plugin_count;
}

HMEMORYMODULE* plugin_get_handles() {
    return plugin_handles;
}

void plugin_cleanup(void* plugin_handle) {
    MemoryFreeLibrary(plugin_handle);
    for (uint32_t i = 0; i < plugin_count; i++) {
        if (plugin_handles[i] == plugin_handle) {
            plugin_handles[i] = NULL;
        }
    }
}

// This casting is really annoying, but it will just throw errors about the slightly different types if I don't.
const plugin_api api = {
        .plugin_cleanup        = plugin_cleanup,
        .PHYSFS_exists         = (bool (*)(const char*)) PHYSFS_exists,
        .PHYSFS_openRead       = (void*(*)(const char*)) PHYSFS_openRead,
        .PHYSFS_openWrite      = (void*(*)(const char*)) PHYSFS_openWrite,
        .PHYSFS_openAppend     = (void*(*)(const char*)) PHYSFS_openAppend,
        .PHYSFS_readBytes      = (int64_t (*)(void*, void*, uint64_t)) PHYSFS_readBytes,
        .PHYSFS_writeBytes     = (int64_t (*)(void*, const void*, uint64_t)) PHYSFS_writeBytes,
        .PHYSFS_fileLength     = (int64_t (*)(void*)) PHYSFS_fileLength,
        .PHYSFS_tell           = (int64_t (*)(void*)) PHYSFS_tell,
        .PHYSFS_seek           = (int32_t (*)(void*, uint64_t)) PHYSFS_seek,
        .PHYSFS_close          = (int (*)(void*)) PHYSFS_close,
        .PHYSFS_enumerateFiles = PHYSFS_enumerateFiles,
        .PHYSFS_freeList       = PHYSFS_freeList,
        .plugin_get_count      = plugin_get_count,
        .plugin_get_handles    = plugin_get_handles,
        .plugin_get_proc_address = (void *(*)(void *, const char *)) MemoryGetProcAddress
};

HMEMORYMODULE custom_load_library(LPCSTR name, void* unused) {
    char path[MAX_PATH] = "/plugins/";
    strncat(path, name, MAX_PATH - sizeof("/plugins/"));
    if (PHYSFS_exists(path)) {
        PHYSFS_File* module_file = PHYSFS_openRead(path);
        if (module_file == PHYSFS_ERR_OK) {
            uint64_t size = PHYSFS_fileLength(module_file);
            uint8_t* buffer = calloc(1, size);
            if (buffer != NULL) {
                PHYSFS_readBytes(module_file, buffer, size);
                PHYSFS_close(module_file);
                return MemoryLoadLibraryEx(buffer, size, MemoryDefaultAlloc, MemoryDefaultFree, custom_load_library, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, NULL);
            }
        }
    }
    // It doesn't exist in the search path or we failed to load the DLL, fall back to the default.
    LoadLibraryA(name);
}

void load_plugins() {
    static const char* dir = "/plugins/";
    char** file_list = PHYSFS_enumerateFiles(dir);

    // Find out how many plugins are in the folder
    for (char**i = file_list; *i != NULL; i++) {
        if (path_has_extension(*i, ".dll")) {
            plugin_count++;
        }
    }
    plugin_handles = calloc(plugin_count, sizeof(*plugin_handles));

    uint32_t idx = 0;
    for (char** i = file_list; *i != NULL; i++) {
        if (path_has_extension(*i, ".dll")) {
            char full_path[MAX_PATH] = {0};

            // Get full virtual filesystem path.
            sprintf(full_path, "%s%s", dir, *i);

            PHYSFS_File* dll_file = PHYSFS_openRead(full_path);
            if (dll_file != NULL) {
                int64_t filesize = PHYSFS_fileLength(dll_file);
                uint8_t* plugin_data = calloc(1, filesize);
                if (plugin_data != NULL) {
                    PHYSFS_readBytes(dll_file, plugin_data, filesize);
                    // This is a really long one-liner, but this is just MemoryLoadLibrary() but using custom_load_library() to load dependencies.
                    plugin_handles[idx] = MemoryLoadLibraryEx(plugin_data, filesize, MemoryDefaultAlloc, MemoryDefaultFree, custom_load_library, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, NULL);

                    PLUGINMAIN plugin_main = (PLUGINMAIN) MemoryGetProcAddress(plugin_handles[idx], "plugin_main");
                    FARPROC dll_main = MemoryGetProcAddress(plugin_handles[idx], "DllMain");
                    if (dll_main != NULL) {
                        if (plugin_main != NULL) {
                            printf("%s: %s exports DllMain() and plugin_main(), both will be executed.\n", loader_warn, *i);
                        }
                        else {
                            // TODO: This is kind of a long message, maybe shorten it?
                            printf("%s: %s doesn't export plugin_main(). Using plugin_main() is preferred, because it gives access to the virtual filesystem and allows you to unload your plugin.\n", loader_warn, *i);
                        }
                    }
                    if (plugin_main != NULL) {
                        plugin_main(plugin_handles[idx], &api);
                    }
                }
            }

            printf("%s: Loaded %s.\n", loader_msg, *i);
            idx++;
        }
    }
    PHYSFS_freeList(file_list);
}
