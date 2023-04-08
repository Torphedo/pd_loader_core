# Phantom Dust Plugin Loader
This is a mod loader for the 2017 Windows port of the 2004 Xbox game Phantom Dust. It heavily relies on DLL
injection to load arbitrary mod code in the game process. Function hooking is also used to intercept/redirect file I/O
from the game and force it to open modded files (so that modders don't need to replace the original files).

DLL injection and the mod loader's [virtual filesystem](https://github.com/icculus/physfs) also provides some extra
options for modders:
- Add extra functionality that runs before or after any function in the game
- Completely replace any function in the game with their own code
- Read and write directly to the game's memory without the overhead and occasional permission problems of WinAPI
- Effortlessly read/write files from the original game, the mods folder, or .zip / .7z archives in the mods folder.
  The virtual filesystem handles file I/O from archives and figures out which of the 3 locations to use automatically.

For those interested, more technical info can be found in the "Plugin Development" section. If you have any problems
or questions, check the FAQ section. There (might) already be an answer there.

## Usage
To start the game with mods, run `pdpl.exe` (found on the [Releases](https://github.com/Torphedo/pd_loader_core/releases) page).
The EXE has been getting flagged a LOT more since I moved a bunch of functionality out to `pd_loader_core.dll`, so you
might need to fight with Windows Defender a little bit to run it. *This will close Phantom Dust if it's already open*.  
You'll know mods are enabled if the version number on the title screen reads `0.00`. Mods are only enabled when
you run the game using `pdpl.exe`. After you run `pdpl.exe` for the first time and you can see that the mods are enabled,
you can delete `pd_loader_core.dll`.

The `mods` folder used by the virtual filesystem is at the following location:   
`%LOCALAPPDATA%\Packages\Microsoft.MSEsper_8wekyb3d8bbwe\RoamingState\mods`   
This folder will be created when you run the plugin manager for the first time, or when you run the included
`setup_junction.bat` script. This script sets up a hard link (similar to a shortcut) that lets you easily access the
mods folder without needing to go to this long folder path every time.

Because creating junctions/hard links requires `Administrator` permissions on Windows 10 (unless Developer Mode is
enabled), I decided to make it into a batch script so that you can read exactly what it does.
The main executable is (intentionally) quite obfuscated, and I didn't want to make a program that's already flagged 
fairly hard by Windows Defender require admin permissions for something so small. I'm not entirely sure if it will ask
for admin permissions, or if you need to right-click it and run it as administrator manually.

If you want to see console output from mods, you'll have to sideload the game by following [this guide](https://phantomdust.miraheze.org/wiki/Help:Dumping_the_game_files).   

Whenever you put anything into the `mods` folder, ***make sure to copy the file(s), not move them***. If you move the
file, it won't be encrypted correctly by Windows and neither the game nor the mods will be able to read it.

# Editing Game Files
To replace a file in the game, place it in the mods folder with the same path it would have in the original game.   
For example: to replace Edgar's third outfit (located at `Assets/Data/Player/pc01a/pc01a2.alr`), you would copy your
edited `pc01a2.alr` to `mods/Assets/Data/player/pc01a/pc01a2.alr`. This only applies to files that you can see being
opened in the console (prefixed with `CreateFile2(): `).   
Files like `.mp4` cutscenes and `.cso` DirectX shaders can't be replaced using this program (yet), but the code which
loads mod files is in this repo at `pd_loader_core/src/hooks.c`.

# Installing Mods
To install a plugin, just copy the DLL file into the `mods/plugins` folder and run `pdpl.exe`. If the plugin came as a
`.zip` or `.7z` file, then copy it to the `mods` folder (*not* `mods/plugins`).

The release includes a plugin you can test with, inside `single_skills.7z`. This mod lets you load custom skill files
and text from the `mods/skills` and `mods/skills/text` folders, without needing to edit a single large file for all of
them. Source code for the mod is available [here](https://github.com/Torphedo/single_skills). To install the mod, copy
the `single_skills.7z` file into the `mods` folder.

The release also includes a simple test skill for you to test that mod. It makes `Rapid Cannon` bouncy and renames it
to `Bullet Hell`. To use it, copy the `bullet_hell.7z` file into the `mods` folder.

Unlike plugins and the files used by them, original game files can't be loaded from `.zip` or `.7z` archives (yet).
This is because of multi-threaded file I/O used by the game for most menus and levels.

## FAQ
Q: It's not allowing me to put files in the mods folder. What's wrong?  
A: The mods folder gets encrypted by Windows and has some restrictions on it, try copying the file instead of moving it
(hold Ctrl while dragging or use Ctrl+C / Ctrl+V).

Q: The game is failing to load the files I put in the mods folder, even though I can see them in File Explorer. What's 
going on?  
A: You probably accidentally moved the file instead of copying it.

Q: I ran setup_junction.bat, but nothing happened. How do I fix it?
A: Try right-clicking the script and selecting "Run as administrator".

Q: Why don't you provide / publish the source code for pdpl.exe?  
A: I want to make it as difficult as possible to cheat in normal multiplayer lobbies using my tool(s). With source code
access, anyone could easily remove all of my anti-cheating measures and recompile it. This is also why I chose to use C
instead of a higher-level language like C#. I understand that being closed-source comes off a bit sketchy to some people
(especially because the main executable is constantly setting off anti-viruses with DLL injection), but I don't feel
that I can safely open-source it without causing damage to the unusually healthy multiplayer scene of this game.

## Plugin Development

The `single_skills.dll` mod essentially serves as an example / proof of concept mod for this mod loader. The source
code is available [here](https://github.com/Torphedo/single_skills).

### Building
Plugins can be written in standard C or C++. Standard output is sent to the plugin console automatically, but I'm not
sure about C++ `std::cout`. If it doesn't work, try running `std::ios_base::sync_with_stdio(true);` first. My program
and test plugins are written in C99, so I haven't tested it.  
The entry point for your plugin is a normal `DllMain()`. Read the [single_skills source code](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/dll_main.c#L203-L211)
or [Microsoft's documentation](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain) for more details.

Note: `pd_loader_core` is normally compiled with MinGW. It might also work on MSVC, but I haven't fully tested it.
It should *at least* compile.

Plugins are manually mapped into the game by [MemoryModule](https://github.com/Torphedo/MemoryModule), so they're not
visible in any module list like normal DLLs are. If you want to access functions from other plugins, you need to use
the [plugin_get_proc_address()](https://github.com/Torphedo/pd_loader_core/blob/main/pd_loader_core/src/plugins.c#L45-L52)
function exported from `pd_loader_core.dll`. See the single_skills source code for an [example](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/imports.h#L17-L29).

### Direct Memory Access
One major benefit DLL injection provides is that your code can directly read/write game memory. Game functions and
variables are stored at a static offset from the main module `PDUWP.exe`. You can use this to hardcode locations into
your code without needing to search for some data structure or piece of code in memory. This is used in `single_skills`
to hook the function that loads skill data/text, and read/write the modded data to a pre-determined location the game
reads it from. ([Direct link to relevant code](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/dll_main.c#L176-L179))

### Hooking
To change the behaviour of a function, use the MinHook library (https://github.com/TsudaKageyu/minhook).
[This](https://github.com/Torphedo/single_skills/blob/master/single_skills/src/dll_main.c#L181-L189) function in the
`single_skills` mod shows how to set up a hook on a game function. The mod hooks a function responsible for loading
skill data/text, then runs its own code afterwards to load in the modded files.  
Hooking isn't useful for everything, but when you need it, it's amazing. Here are some ideas for using hooks in a mod:
- Make a freecam by overriding the camera controller
- Intercept some part of the game's Direct3D rendering and render your own extra graphics on top
- Rewrite a function that loads 3D models to use a common format like `.gltf` or `.obj`
- Intercept a function processing input data to make your own keybinds or completely disable certain buttons
- Intercept calls from DirectX to make the game load shaders from source code (instead of `.cso` bytecode) and compile them at runtime

### Filesystem
The `PHYSFS_*` functions give you access to the [PhysicsFS](https://github.com/icculus/physfs) virtual filesystem,
which lets you read data from the original game, the mods folder, or zip files with no extra effort. PhysicsFS will
automatically figure out which location to use in the following order of priority:
```
    1. Files inside .zip or .7z archives, in alphabetical order by archive name
    2. Loose files in the mods folder
    3. Original game folder
```
Keep in mind that `.` and `..` directories are not allowed in PhysicsFS calls and will fail. For documentation on
exactly what each PhysicsFS function does, search for a function in their
[documentation](http://www.icculus.org/physfs/docs/html/physfs_8h.html).

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
