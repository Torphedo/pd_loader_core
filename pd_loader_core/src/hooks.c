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

#include "hooks.h"
#include "path.h"

// Definitions for the functions we need to hook
typedef HANDLE (*CREATE_FILE_2)(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS);
CREATE_FILE_2 original_CreateFile2 = NULL;
CREATE_FILE_2 addr_CreateFile2 = NULL;

HANDLE open_mutex = NULL;

HANDLE hook_CreateFile2(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams) {
  	uint32_t wait_result = WaitForSingleObject(open_mutex, INFINITE);

  	// We got ownership of the mutex
	if (wait_result == WAIT_OBJECT_0) {
	  char path[MAX_PATH] = {0};
	  wchar_t wide_path[MAX_PATH] = {0};
	  PHYSFS_utf8FromUtf16(lpFileName, path, MAX_PATH);
	  path_make_physfs_friendly(path);

	  HANDLE win_handle = NULL;

	  // Try to open the file with the default function.
	  if (PHYSFS_exists(path)) {
		// Get the real path of the file and place it in a wide string.
		swprintf(wide_path, MAX_PATH, L"%hs/%hs", PHYSFS_getRealDir(path), path);
		win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);

		// The file doesn't exist, make it using the data from the PhysicsFS file.
		if (win_handle == INVALID_HANDLE_VALUE) {
		  static char fake_path[MAX_PATH] = {0};
		  get_ms_esper_path(fake_path);
		  {
			static char filename[MAX_PATH] = {0}; // Temporary buffer to store filename
			path_get_filename(path, filename);
			sprintf(fake_path, "%sfake\\%s", fake_path, filename);
		  }

		  // Create a new file with the appropriate size in the /fake/ folder.
		  PHYSFS_File* archive_file = PHYSFS_openRead(path);
		  uint64_t size = PHYSFS_fileLength(archive_file);
		  FILE* fake_file = fopen(fake_path, "wb");
		  if (fake_file != NULL) {
			uint8_t* file_data = malloc(size);
			if (file_data == NULL) {
			  fclose(fake_file);
			}
			else {
			  PHYSFS_readBytes(archive_file, file_data, size);
			  fwrite(file_data, size, 1, fake_file);
			  fclose(fake_file);
			  free(file_data);
			}
		  }

		  PHYSFS_utf8ToUtf16(fake_path, wide_path, MAX_PATH);
		  printf("Opening a mod file for Windows at %ls.\n", wide_path);
		  win_handle = original_CreateFile2(wide_path, dwDesiredAccess, dwShareMode, OPEN_EXISTING, pCreateExParams);
		}
	  } else {
		// Give up and just use the original filepath if it's not in PHYSFS.
		win_handle = original_CreateFile2(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, pCreateExParams);
		// printf("[VANILLA CALL] ");
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

	  // Uncomment this to watch files open in slow motion and make sure the mutex is working.
	  // system("pause");
	  if (!ReleaseMutex(open_mutex)) {
		printf("CreateFile2(): Failed to release mutex!\n");
	  }
	  return win_handle;
	}
	else if (wait_result == WAIT_ABANDONED) {
	  printf("CreateFile2(): Open mutex abandoned. Panic time!\n");
	  return 0;
	}
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
