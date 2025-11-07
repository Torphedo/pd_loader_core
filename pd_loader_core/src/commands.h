#pragma once
#include <stdbool.h>

typedef int (*command_func)(int argc, char** argv);

// Initialize internal state for the command system
bool command_sys_init();
void command_sys_deinit();

void plugin_register_command(command_func command, const char* module_name, const char* command_name);

int exec_command(const char* command);
