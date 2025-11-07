#pragma once
#include <stdbool.h>

typedef int (*command_func)(int argc, char** argv);

// Initialize internal state for the command system
bool command_sys_init();
void command_sys_deinit();

/// @brief Register a function as a command named "[module_name]![command_name]"
///
/// e.g. command_register(func, "tools", "help") creates the command "tools!help".
/// @param command The function to call when the command is run
/// @param module_name The name of your plugin DLL (or a shorthand for it)
/// @param command_name The name of your command
void command_register(command_func command, const char* module_name, const char* command_name);

int command_exec(const char* command);

/// @brief Parse Linux-style options with values
///
/// For example, if you want to specify a filepath, you might have an option
/// named "--path", with "-p" as a shorthand. This function will handle:
///   --path /path/to/file
///   --path "/path/to/file"
///   -p /path/to/file
///   -p "/path/to/file"
/// @param argc Your argc
/// @param argv Your argv
/// @param option Long form of your option name
/// @param shorthand Short form of your option name
/// @return String value, or NULL if not found
char* command_getoption(int argc, char** argv, const char* option, const char* shorthand);

/// @brief Parse Linux-style boolean flags
///
/// For example, you might have an option named "--skip", with "-s" as a
/// shorthand. This function will succeed if it finds either.
/// @param argc Your argc
/// @param argv Your argv
/// @param option Long form of your option name
/// @param shorthand Short form of your option name
/// @return Whether the flag was present
bool command_getflag(int argc, char** argv, const char* option, const char* shorthand);
