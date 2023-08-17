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

HCUSTOMMODULE custom_load_library(const char* filename, void* userdata) {
    char vpath[MAX_PATH] = {0};

    // Early exit if it's already loaded.
    HCUSTOMMODULE handle_out = plugin_get_module_handle(filename);
    if (handle_out != NULL) {
        return handle_out;
    }

    sprintf(vpath, "/plugins/%s", filename);
    if (PHYSFS_exists(vpath)) {
        handle_out = vfs_load_dll(vpath);
    }
    else {
        handle_out = LoadLibraryA(filename);
    }
    return handle_out;
}

FARPROC custom_get_proc_address(HCUSTOMMODULE module, const char* name, void* userdata) {
    FARPROC addr = GetProcAddress(module, name);
    if (addr == NULL) {
        addr = MemoryGetProcAddress(module, name);
    }
    return addr;
}

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

void* plugin_get_proc_address(void* handle, const char* function_name) {
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

void* vfs_load_dll(char* filename) {
    void* handle_out = plugin_get_module_handle(filename);

    printf("%s: Loading %s...\n", loader_msg, filename);
    // It's already loaded in the list. Return NULL handle so caller knows something went wrong.
    if (handle_out != NULL) {
        printf("%s: %s is already loaded.\n", loader_warn, filename);
        return NULL;
    }

    char vpath[MAX_PATH] = {0};

    // Create full VFS path.
    sprintf(vpath, "/plugins/%s", filename);

    // Read file into a buffer and load it with MemoryModule.
    PHYSFS_File* dll_file = PHYSFS_openRead(vpath);
    if (dll_file == NULL) {
        printf("%s: Failed to open %s for reading (PHYSFS).\n", loader_err, vpath);
    }
    else {
        int64_t filesize = PHYSFS_fileLength(dll_file);
        uint8_t *plugin_data = calloc(1, filesize);
        if (plugin_data == NULL) {
            printf("%s: Failed to allocate %lli bytes for %s (PHYSFS).\n", loader_err, filesize, vpath);
        } else {
            PHYSFS_readBytes(dll_file, plugin_data, filesize);

            // Load the library, using our custom LoadLibrary and GetProcAddress code to resolve imports.
            handle_out = MemoryLoadLibraryEx(plugin_data, filesize, MemoryDefaultAlloc, MemoryDefaultFree, custom_load_library,
                                             custom_get_proc_address, MemoryDefaultFreeLibrary, NULL);

            // MemoryModule automatically calls the DLL/EXE entry point.
            printf("%s: Done.\n", loader_msg, filename);
        }
    }
    return handle_out;
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

            loaded_modules[idx].handle = vfs_load_dll(*i);

            // Copy the filename into the structure.
            if (loaded_modules[idx].handle != NULL) {
                loaded_modules[idx].filename = calloc(1, strlen(*i));
                strcpy(loaded_modules[idx].filename, *i);
            }

            idx++;
        }
    }
    PHYSFS_freeList(file_list);
}
