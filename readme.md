# Phantom Dust Plugin Loader
This is a mod loader for the 2017 Windows port of the 2004 Xbox game Phantom Dust. It uses DLL
injection to load code and asset mods without needing read/write access to the game files.


## Usage
Download PDPL from [here](https://github.com/Torphedo/pd_loader_core/releases), then unzip it to a
folder and run `pdpl.exe`. This will create a shortcut to the mods folder, then run Phantom Dust.
Don't create a `mods` folder yourself, because the one PDPL creates is actually a junction (like
a shortcut) to `%LOCALAPPDATA%\Packages\Microsoft.MSEsper_8wekyb3d8bbwe\RoamingState\mods`. This
path is the only place where the game is allowed to load files from.     

Most mods are `.zip` or `.7z` files, which you can copy into the `mods` folder without needing to unzip
them. The release comes with an example mod, `DebugMenu.7z`. If you install this to the `mods` folder
and run `pdpl.exe` again, Phantom Dust will close and re-open. Once it boots up, you should see some
new menu options throughout the game. If you run the game again without PDPL, everything will be back to normal.

If you want to see console output from mods, you'll have to sideload the game by following [this guide](https://phantomdust.miraheze.org/wiki/Help:Dumping_the_game_files).   

Whenever you put anything into the `mods` folder, ***make sure to copy the file(s), not move them***. If you move the
file, it won't have its permissions set correctly by Windows, and neither the game nor the mods will be able to read it.

# Editing Game Files
To replace a file in the game, place it in the mods folder with the same path it would have in the original game.   
For example: to replace Edgar's third outfit (located at `Assets/Data/Player/pc01a/pc01a2.alr`), you would copy your
edited `pc01a2.alr` to `mods/Assets/Data/player/pc01a/pc01a2.alr`. This only applies to files that you can see being
opened in the console (prefixed with `CreateFile2(): `).   
Files like `.mp4` cutscenes and `.cso` DirectX shaders can't be replaced using this program (yet), but the code which
loads mod files is in this repo at `pd_loader_core/src/hooks.c`. Paths in the mod loader are *case sensitive*! Usually,
the `Assets/Data/` part of the path has the first letter capitalized, and everything else (like `/map/`) is in lowercase.
If you're not sure, look at the console to see what path the game is trying to open.

# Installing Mods
To install a plugin, just copy the DLL file into the `mods/plugins` folder and run `pdpl.exe`. If the plugin came as a
`.zip` or `.7z` file, then copy it to the `mods` folder (*not* `mods/plugins`), and don't unzip it.

## FAQ
Q: It's not allowing me to put files in the mods folder. What's wrong?  
A: The mods folder gets encrypted by Windows and has some restrictions on it, try copying the file instead of moving it
(hold Ctrl while dragging or use Ctrl+C / Ctrl+V).

Q: The game is failing to load the files I put in the mods folder, even though I can see them in File Explorer. What's 
going on?  
A: You probably accidentally moved the file instead of copying it.

## Plugin Development
Using PDPL, you can:
- Add extra functionality that runs before or after any function in the game
- Completely replace any function in the game with custom code
- Read and write directly to the game's memory without the overhead of the Windows API
- Seamlessly read/write files from the original game, the mods folder, or .zip / .7z archives in the mods folder.
  The virtual filesystem handles file I/O from archives and figures out which of the 3 locations to use automatically.

The `single_skills.dll` mod essentially serves as an example / proof of concept mod for this mod loader. The source
code is available [here](https://github.com/Torphedo/single_skills).

Plugins can be written in standard C or C++. Standard output is sent to the plugin console automatically, but I'm not
sure about C++ `std::cout`. If it doesn't work, try running `std::ios_base::sync_with_stdio(true);` first. My program
and test plugins are written in C99, so I haven't tested it.  
The entry point for your plugin is a normal `DllMain()`. Read the [single_skills source code](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/dll_main.c#L203-L211)
or [Microsoft's documentation](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain) for more details.

Plugins are manually mapped into the game by [MemoryModule](https://github.com/Torphedo/MemoryModule), so they're not
visible in any module list like normal DLLs are. However, you can still import functions from other plugin DLLs or from
`pd_loader_core.dll` by linking against them. The `plugin_dev.zip` file has a copy of the `.a` import library you'll need
to link with the core loader, and headers to access the filesystem API. The `single_skills` project has an example of
this linking.

### Direct Memory Access
In a plugin, you can directly read/write game memory. Game functions and statically allocated data are stored at a
static offset from the main module `PDUWP.exe`. You can use this to hardcode locations into your code without needing to
search for some data structure or piece of code in memory. This is used in `single_skills` to hook the function that
loads skill data/text, and read/write the modded data to a pre-determined location the game reads it from.
([Direct link to relevant code](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/dll_main.c#L176-L179))

### Hooking
To change the behaviour of a function, use the MinHook library (https://github.com/TsudaKageyu/minhook).
[This](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/dll_main.c#L181-L189) function in the
`single_skills` mod shows how to set up a hook on a game function. The mod hooks a function responsible for loading
skill data/text, then runs its own code afterwards to load in the modded files.
Hooking isn't useful for everything, but when you need it, it's amazing. Here are some ideas for using hooks in a mod:
- Make a freecam by hooking the camera controller
- Intercept some part of the game's Direct3D rendering and render your own extra graphics on top
- Rewrite a function that loads 3D models to use a common format like `.gltf` or `.obj`
- Intercept a function processing input data to make your own keybinds or completely disable certain buttons
- Intercept calls from DirectX to make the game load shaders from source code (instead of `.cso` bytecode) and compile them at runtime
- Reload the map on-demand

### Filesystem
The `PHYSFS_*` functions give you access to the [PhysicsFS](https://github.com/icculus/physfs) virtual filesystem,
which lets you read data from the original game, the mods folder, or zip files with no extra effort. PhysicsFS will
automatically figure out which location to use in the following order of priority:
```
    1. Files inside .zip or .7z archives, in alphabetical order by archive name
    2. Loose files in the mods folder
    3. Original game folder
```
Because of the mod loader's function hooks, you'll also be able to read data from zip files using the standard C/C++
file I/O interfaces. If using PhysicsFS directly, keep in mind that `.` and `..` directories are not allowed in the
VFS paths and will fail. For documentation on exactly what each PhysicsFS function does, search for a function in
the header file, or their [documentation](http://www.icculus.org/physfs/docs/html/physfs_8h.html).

By default, the write directory for PhysicsFS is the `RoamingState` folder which contains the `mods` folder.

Other archive formats supported by the library are:
- .iso images
- .vdf (Gothic/Gothic II)
- .slb (Independence War)
- .grp (Build Engine)
- .pak (Quake / Quake 2)
- .hog (Descent / Descent 2)
- .mvl (Descent / Descent 2)
- .wad (DOOM)

To save space, I disabled support for all archives except `.zip` and `.7z`. If you want me to enable one of the
available formats (or you have some custom archiver), let me know and I can add it (or you can enable it [here](https://github.com/Torphedo/pd_loader_core/blob/main/CMakeLists.txt#L19-L27)
and rebuild `pd_loader_core` yourself).
