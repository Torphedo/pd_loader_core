#pragma once

static const char vfs_msg[] = "[\033[32mVirtual Filesystem\033[0m]";
static const char vfs_err[] = "[\033[31mVirtual Filesystem\033[0m]";

// Setup PhysicsFS and mount everything in the following order:
// - Mod folder
// - All archives in the mod folder
// - Vanilla game folder
void vfs_setup();
