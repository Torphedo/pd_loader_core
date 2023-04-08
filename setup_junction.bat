@echo off
echo This script may need to be run as Administrator to work on Windows 10,
echo unless you've enabled Developer Mode. Since this is the only thing needing
echo admin access, I've made it into a separate script so you can read what it
echo does yourself.
echo.
echo Press any key to proceed with creating the mods folder as a hard link...
pause > NUL

set mods_plugins_folder="%LOCALAPPDATA%\Packages\Microsoft.MSEsper_8wekyb3d8bbwe\RoamingState\mods\plugins"
set mods_skills_text_folder="%LOCALAPPDATA%\Packages\Microsoft.MSEsper_8wekyb3d8bbwe\RoamingState\mods\skills\text"

rem Create the mod folders if they don't exist
if not exist %mods_plugins_folder% (
   mkdir %mods_plugins_folder%
)

if not exist %mods_skills_text_folder% (
   mkdir %mods_skills_text_folder%
)

rem Create the hard link
rem I believe this is the point where it will (probably) ask for admin permissions.
rem If it doesn't ask and the junction doesn't get created, try running the script as Administrator.

mklink /J "mods" "%LOCALAPPDATA%\Packages\Microsoft.MSEsper_8wekyb3d8bbwe\RoamingState\mods"
echo Created hard link.
pause
