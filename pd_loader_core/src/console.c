#include <stdio.h>

#include <Windows.h>
#include <processenv.h>

#include "console.h"

bool console_setup(int16_t min_height) {
    if (AllocConsole()) {
        // Set the screen buffer height for the console
        CONSOLE_SCREEN_BUFFER_INFO console_info = {0};
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
        if (console_info.dwSize.Y < min_height) {
            console_info.dwSize.Y = min_height;
        }
        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), console_info.dwSize);

        // Enable ANSI escape codes
        uint32_t ConsoleMode = 0;
        GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), (LPDWORD) &ConsoleMode);
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

        // Show the console window.
        HWND window = FindWindowA("ConsoleWindowClass", NULL);
        ShowWindow(window, SW_SHOW);

        return console_redirect_stdio();
    }
}

bool console_redirect_stdio() {
    bool result = true;
    FILE *fp;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE) {
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0) {
            result = false;
        } else {
            setvbuf(stdin, NULL, _IONBF, 0);
        }
    }
    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE) {
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0) {
            result = false;
        } else {
            setvbuf(stdout, NULL, _IONBF, 0);
        }
    }
    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE) {
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0) {
            result = false;
        } else {
            setvbuf(stderr, NULL, _IONBF, 0);
        }
    }
    return result;
}
