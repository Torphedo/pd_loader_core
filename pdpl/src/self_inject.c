#include <stdlib.h>

#include "self_inject.h"

typedef struct BASE_RELOCATION_ENTRY {
	USHORT Offset : 12;
	USHORT Type : 4;
}BASE_RELOCATION_ENTRY;

bool self_inject(uint32_t process_id, LPTHREAD_START_ROUTINE entry_point) {
    // Open the target process - this is the process we will be injecting this PE into
    HANDLE target_process = OpenProcess(PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION, FALSE, process_id);
    if (target_process == INVALID_HANDLE_VALUE) {
  	    return false;
    }

    // Get current image's base address
    uint8_t* image_base = (uint8_t*) GetModuleHandle(NULL);
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*) image_base;
    IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*) (image_base + dos_header->e_lfanew);

    // Allocate buffers for the PE data in the local and remote processes
    uint8_t* local_image = malloc(nt_header->OptionalHeader.SizeOfImage);
    // We use a non-pointer type to avoid dereferncing remote memory. We need the
    // target address from the start for relocations.
    uintptr_t target_image = (uintptr_t)VirtualAllocEx(target_process, NULL, nt_header->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    // Early exit if we fail an allocation. Free does nothing if passed NULL.
    if (local_image == NULL || target_image == 0) {
        free(local_image);
        VirtualFreeEx(target_process, (void*)target_image, nt_header->OptionalHeader.SizeOfImage, MEM_RELEASE);
        CloseHandle(target_process);
        return false;
    }

    // Copy PE data into our local buffer
    memcpy(local_image, image_base, nt_header->OptionalHeader.SizeOfImage);

    // Calculate difference between where the image was originally loaded and the target location
    uintptr_t reloc_bias = (uintptr_t)target_image - (uintptr_t)image_base;

    // Relocate local_image, to ensure that it will have correct addresses once it's in the target process
    IMAGE_BASE_RELOCATION* reloc_table = (IMAGE_BASE_RELOCATION*)((uintptr_t)local_image + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

    while (reloc_table->SizeOfBlock > 0) {
        // BASE_RELOCATION_ENTRY is 2 bytes, so the size (minus header) over 2 is our entry count
        // (not using sizeof(BASE_RELOCATION_ENTRY) because some compilers might add padding or something on bitfields)
        uint32_t entries_count = (reloc_table->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
        // This VirtualAddress member tells us the offset of the relocation table from the image base.
        BASE_RELOCATION_ENTRY* entries = (BASE_RELOCATION_ENTRY*)((uintptr_t)reloc_table + sizeof(IMAGE_BASE_RELOCATION));

        for (uint32_t i = 0; i < entries_count; i++) {
              if (entries[i].Offset) {
                  // Get the address of the pointer we need to relocate, then add our image base delta.
                  uintptr_t* patched_address = (uintptr_t*)((uintptr_t)local_image + reloc_table->VirtualAddress + entries[i].Offset);
                  *patched_address += reloc_bias + reloc_table->VirtualAddress;
              }
        }

        // Go to the next relocation table and repeat. There is one table per 4KiB page.
        reloc_table = (IMAGE_BASE_RELOCATION*)((uintptr_t)reloc_table + reloc_table->SizeOfBlock);
    }

    // Write the relocated PE into the target process
    WriteProcessMemory(target_process, (void*)target_image, local_image, nt_header->OptionalHeader.SizeOfImage, NULL);
    free(local_image);

    // Start the injected PE inside the target process
    CreateRemoteThread(target_process, NULL, 0, (LPTHREAD_START_ROUTINE)((uintptr_t)entry_point + reloc_bias), (void*)target_image, 0, NULL);
    CloseHandle(target_process);

    return true;
}