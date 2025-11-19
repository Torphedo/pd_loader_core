#pragma once

#define vfs_msg "[\033[32mVirtual Filesystem\033[0m]: "
#define vfs_err "[\033[31mVirtual Filesystem\033[0m]: "

// Setup PhysicsFS and mount everything in the following order:
// - Mod folder
// - All archives in the mod folder
// - Vanilla game folder
void vfs_setup();

/// @brief Copy a file from the VFS to a real path on disk
bool dump_vfs_file(const char* vfs_path, const char* real_path);
