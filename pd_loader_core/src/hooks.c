#include <stdio.h>

// Allow use of LPCREATEFILE2_EXTENDED_PARAMETERS on MinGW
#ifdef _WIN32_WINNT
    #if(_WIN32_WINNT <= 0x601)
        #undef _WIN32_WINNT
        #define _WIN32_WINNT 0x0602
    #endif // #if(_WIN32_WINNT <= 0x601)
#endif // #ifdef _WIN32_WINNT

#include <Windows.h>

#include <fileapi.h>
#include <MinHook.h>
#include <physfs.h>
#include <common/path.h>
#include <common/logging.h>

#include "hooks.h"
#include "pd_path.h"

// Definitions for the functions we need to hook
typedef HANDLE (*CREATE_FILE_2)(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS);
CREATE_FILE_2 original_CreateFile2 = NULL;
CREATE_FILE_2 addr_CreateFile2 = NULL;

HANDLE open_mutex = NULL;

HANDLE hook_CreateFile2(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams) {
      u32 wait_result = WaitForSingleObject(open_mutex, INFINITE);
      if (wait_result != WAIT_OBJECT_0) {
          printf("CreateFile2(): Something's gone wrong with the VFS mutex!\n");
          return 0;
      }

      char path[MAX_PATH] = {0};
      PHYSFS_utf8FromUtf16(lpFileName, path, MAX_PATH);
      path_make_physfs_friendly(path);

      HANDLE win_handle = NULL;

      // Try to open the file with the default function.
      if (PHYSFS_exists(path)) {
          wchar_t wide_path[MAX_PATH] = {0};
          // Get the real path of the file and place it in a wide string.
          swprintf(wide_path, MAX_PATH, L"%hs/%hs", PHYSFS_getRealDir(path), path);
          win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);

          // The file doesn't exist, make it using the data from the PhysicsFS file.
          if (win_handle == INVALID_HANDLE_VALUE) {
              static char fake_path[MAX_PATH] = {0};
              get_fake_file_path(fake_path);
              CreateDirectoryA(fake_path, NULL); // Make sure the folder exists
              {
                  static char filename[MAX_PATH] = {0}; // Temporary buffer to store filename
                  path_get_filename(path, filename);
                  sprintf(fake_path, "%s\\%s", fake_path, filename);
              }

              // Create a new file with the appropriate size in the /fake/ folder.
              PHYSFS_File* archive_file = PHYSFS_openRead(path);
              uint64_t size = PHYSFS_fileLength(archive_file);
              FILE* fake_file = fopen(fake_path, "wb");
              if (fake_file != NULL) {
                  uint8_t *file_data = malloc(size);
                  if (file_data == NULL) {
                      LOG_MSG(error, "Failed to create temporary file '%s' to support zipmod file '%s'.\n", fake_path, path);
                      fflush(stdout);
                      fclose(fake_file);
                  } else {
                      PHYSFS_readBytes(archive_file, file_data, size);
                      fwrite(file_data, size, 1, fake_file);
                      fclose(fake_file);
                      free(file_data);

                      PHYSFS_utf8ToUtf16(fake_path, wide_path, MAX_PATH);
                      win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, OPEN_EXISTING, pCreateExParams);
                  }
              }
          }
      } else {
          // Give up and just use the original filepath if it's not in PHYSFS.
          win_handle = original_CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
      }

    // Colors the "CreateFile2" message green for read, red for write, and yellow for read/write.
    switch (dwDesiredAccess) {
        case GENERIC_READ:
            printf("\033[32m");
            break;
        case GENERIC_WRITE:
            printf("\033[31m");
            break;
        case (GENERIC_READ | GENERIC_WRITE):
            printf("\033[33m");
            break;
        default:
            // Just print in white for other permissions
            break;
    }
    printf("CreateFile2");
    printf("\033[0m(): "); // Reset to white before printing "()"
    printf("Opening %s\n", path);
    // Allows us to see if we're opening from a zip file.
    // printf("Opening %s [from real path %s]\n", path, PHYSFS_getRealDir(path));

    if (!ReleaseMutex(open_mutex)) {
        printf("CreateFile2(): Failed to release mutex!\n");
    }
    return win_handle;
}

void hooks_unlock_filesystem() {
  if (!ReleaseMutex(open_mutex)) {
	printf("Failed to release filesystem mutex!\n");
  }
  else {
	printf("Unlocking filesystem...\n");
  }
}

void* get_procedure_address(const wchar_t* module_name, const char* proc_name) {
    HMODULE hModule = GetModuleHandleW(module_name);
    if (hModule == NULL) {
        return NULL;
    }
    return GetProcAddress(hModule, proc_name);
}

bool hooks_setup_lock_files() {
    MH_Initialize();

    addr_CreateFile2 = get_procedure_address(L"KERNELBASE.dll", "CreateFile2");

    if (MH_CreateHook(addr_CreateFile2, &hook_CreateFile2, (void**)&original_CreateFile2) != MH_OK) {
        printf("Failed to create hook.\n");
        return false;
    }

	// Creates the mutex for the CreateFile2() hook and takes ownership until released by hooks_unlock_filesystem()
	// This prevents the game from starting because it can't open any files until our plugin DLLs are done.
	open_mutex = CreateMutex(NULL, TRUE, NULL);
    if (MH_EnableHook(addr_CreateFile2) != MH_OK) {
        printf("Failed to enable hook.\n");
        return false;
    }

    return true;
}
