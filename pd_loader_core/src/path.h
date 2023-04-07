#include <stdbool.h>
#include <stdint.h>

void get_ms_esper_path(char* string);

bool path_has_extension(const char* path, const char* extension);

/// Replaces all backslashes in a string with forward slashes.
/// \param path String to edit. Must be null-terminated.
void path_fix_backslashes(char* path);

/// Truncates a filename or folder name from a path, leaving a trailing "\\" or "/".
/// \param path The path to truncate. This string will be edited. If the string does not contain any slashes or backslashes, it will be completely filled with null characters.
/// \param pos The position to start searching for directory separators. This should usually be the string's length + 1.
void path_truncate(char* path, uint16_t pos);

// Turn all backslashes into forward slashes and remove "." and ".." paths. The input string must be MAX_PATH characters long.
void path_make_physfs_friendly(char* path);

void path_get_filename(const char* path, char* output);
