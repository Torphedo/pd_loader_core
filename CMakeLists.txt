cmake_minimum_required(VERSION 3.16)
project(pd_loader_core
	VERSION 1.1.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD 99) # Our code is C99
set(CMAKE_SHARED_LIBRARY_PREFIX "") # Remove "lib" prefix from the output filename
if (MSVC)
	# Statically link C runtime. Comment this out to get a tiny DLL.
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Build PhysicsFS without docs, to reduce build time.
set(PHYSFS_BUILD_DOCS FALSE)
set(PHYSFS_BUILD_STATIC TRUE)
set(PHYSFS_BUILD_SHARED FALSE)

# Disable everything but zip and 7z support. This saves about 12KiB.
set(PHYSFS_ARCHIVE_GRP FALSE)
set(PHYSFS_ARCHIVE_WAD FALSE)
set(PHYSFS_ARCHIVE_MVL FALSE)
set(PHYSFS_ARCHIVE_HOG FALSE)
set(PHYSFS_ARCHIVE_SLB FALSE)
set(PHYSFS_ARCHIVE_VDF FALSE)
set(PHYSFS_ARCHIVE_QPAK FALSE)
set(PHYSFS_ARCHIVE_ISO9660 FALSE)

# Compile external libraries first, using the CMakeLists.txt in each folder
add_subdirectory(ext/minhook)
add_subdirectory(ext/MemoryModule)
add_subdirectory(ext/physfs)
add_subdirectory(ext/bobtail)

add_library(pd_loader_core SHARED
	pd_loader_core/src/dll_main.c
	pd_loader_core/src/plugins.c
    pd_loader_core/src/pd_path.c
	pd_loader_core/src/filesystem.c
	pd_loader_core/src/console.c
	pd_loader_core/src/hooks.c
    pd_loader_core/src/commands.c
)

target_link_libraries(pd_loader_core PRIVATE "-s" minhook physfs-static MemoryModule bobtail)
target_include_directories(pd_loader_core PRIVATE "../ext/minhook/include" "../ext/physfs/src" "../ext/MemoryModule")

add_executable(pdpl
	pdpl/src/main.c
	pdpl/src/dll.c
	pdpl/src/process.c
)
target_link_libraries(pdpl PRIVATE bobtail)
