#pragma once
#include <stdbool.h>
#include <stdint.h>

// Reduce the size of Windows.h to improve compile time
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOMB
#include <windows.h>

/// Inject the current executable into another process and call the specified function.
/// \param process_id ID of the process you want to inject into
/// \param entry_point Function pointer to the start routine you want to use. THIS NEEDS TO BE A REAL LPTHREAD_START_ROUTINE OR IT WILL CRASH ON RETURN!
/// \return boolean success/failure.
bool self_inject(uint32_t process_id, LPTHREAD_START_ROUTINE entry_point);
