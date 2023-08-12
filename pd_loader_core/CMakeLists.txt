add_library(pd_loader_core SHARED
	"src/dll_main.c"
	"src/plugins.c"
	"src/path.c"
	"src/filesystem.c"
	"src/console.c"
	"src/hooks.c"
)
# Add the MinHook library to our DLL
target_link_libraries(pd_loader_core PRIVATE "-s" minhook physfs-static MemoryModule)

# This lets us include MinHook.h without the whole relative path.
target_include_directories(pd_loader_core PRIVATE "../ext/minhook/include" "../ext/physfs/src" "../ext/MemoryModule")
