#pragma once
#include <stdbool.h>
#include <stdint.h>

void* plugin_get_module_handle(const char* filename);
void* plugin_get_proc_address(const char* filename, const char* function_name);
void load_plugins();
