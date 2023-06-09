#include <stdio.h>
#include <malloc.h>

// Reduce the size of Windows.h to improve compile time.
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

static const char loader_msg[] = "[\033[32mPlugin Loader\033[0m]";  // Green
static const char loader_warn[] = "[\033[33mPlugin Loader\033[0m]"; // Yellow
static const char loader_err[] = "[\033[31mPlugin Loader\033[0m]";  // Red

// We use this to pair the filename with a handle, to implement plugin_get_module_handle().
typedef struct {
    char* filename;
    HMEMORYMODULE handle;
}module;

static module* loaded_modules = NULL;
uint32_t module_count = 0;

void* plugin_get_module_handle(const char* filename) {
    for (uint32_t i = 0; i < module_count + 10; i++) {
        if (loaded_modules[i].handle != NULL) {
            if (strcmp(filename, loaded_modules[i].filename) == 0) {
                // Filename match!
                return loaded_modules[i].handle;
            }
        }
    }

    // No valid handles match this filename.
    return NULL;
}

void* plugin_get_proc_address(const char* filename, const char* function_name) {
    HMEMORYMODULE handle = plugin_get_module_handle(filename);
    if (handle == NULL) {
        // This plugin isn't loaded.
        return NULL;
    }
    return MemoryGetProcAddress(handle, function_name);
}

void plugin_cleanup(void* plugin_handle) {
    if (plugin_handle != NULL) {
        // Remove the old handle from the list.
        for (uint32_t i = 0; i < module_count; i++) {
            if (loaded_modules[i].handle == plugin_handle) {
                loaded_modules[i].handle = NULL;
            }
        }

        MemoryFreeLibrary(plugin_handle);
    }
}

void load_plugins() {
    static const char* dir = "/plugins/";
    char** file_list = PHYSFS_enumerateFiles(dir);

    // Find out how many plugins are in the folder
    for (char**i = file_list; *i != NULL; i++) {
        if (path_has_extension(*i, ".dll") || path_has_extension(*i, ".exe")) {
            module_count++;
        }
    }
    // Allocate enough memory to store information for every module, plus some wiggle room just in case.
    loaded_modules = calloc(module_count + 10, sizeof(*loaded_modules));

    uint32_t idx = 0;
    for (char** i = file_list; *i != NULL; i++) {
        if (path_has_extension(*i, ".dll") || path_has_extension(*i, ".exe")) {
            char full_path[MAX_PATH] = {0};

            // Get full virtual filesystem path.
            sprintf(full_path, "%s%s", dir, *i);

            // Read file into a buffer and load it with MemoryModule.
            PHYSFS_File* dll_file = PHYSFS_openRead(full_path);
            if (dll_file == NULL) {
                printf("%s: Failed to open %s for reading (PHYSFS).\n", loader_err, full_path);
            }
            else
            {
                int64_t filesize = PHYSFS_fileLength(dll_file);
                uint8_t* plugin_data = calloc(1, filesize);
                if (plugin_data == NULL) {
                    printf("%s: Failed to allocate %lli bytes for %s (PHYSFS).\n", loader_err, filesize, full_path);
                }
                else {
                    PHYSFS_readBytes(dll_file, plugin_data, filesize);
                    // This is a really long one-liner, but this is just MemoryLoadLibrary() but using custom_load_library() to load dependencies.
                    loaded_modules[idx].handle = MemoryLoadLibrary(plugin_data, filesize);

                    // Copy the filename into the structure.
                    if (loaded_modules[idx].handle != NULL) {
                        loaded_modules[idx].filename = calloc(1, strlen(*i));
                        strcpy(loaded_modules[idx].filename, *i);
                    }
                    // MemoryLoadLibraryEx automatically calls the DLL/EXE entry point.
                    printf("%s: Loaded %s.\n", loader_msg, *i);
                }
            }
            idx++;
        }
    }
    PHYSFS_freeList(file_list);
}
