#pragma once

#include <stdint.h>
#include <stdbool.h>

/// @brief Give the process a console window and show it
///
/// Automatically redirects stdout, stdin, and stderr to the console and enables
/// ANSI escape codes.
bool console_setup(int16_t min_height);

bool console_redirect_stdio();
