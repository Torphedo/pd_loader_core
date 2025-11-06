#include <stdbool.h>
#include <stdint.h>

/// @brief Get path of the RoamingState folder the game has write permission for
/// @param string Output buffer
void get_roaming_state_path(char* string);

/// @brief Get path of the folder containing PDUWP.exe
void get_pd_path(char* out);

// Turn all backslashes into forward slashes and remove "." and ".." paths. The input string must be MAX_PATH characters long.
void path_make_physfs_friendly(char* path);
