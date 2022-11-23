@echo off

set EXE_NAME=Game.exe

set SDL_DIR=C:\SDL2-2.0.10\
set IMGUI_DIR=..\imgui
set STB_DIR=..\stb
set GLAD_DIR=..\glad

:: Debug messages etc
set ADDITIONAL_FLAGS=/DDEBUG /DTEST -DPLATFORM_GL_MAJOR_VERSION=3 -DPLATFORM_GL_MINOR_VERSION=3

:: Disabled warnings
:: /wd4100  unreferenced formal parameter
:: /wd4189  local variable is initialized but not referenced
:: /wd4996  'strncat': This function or variable may be unsafe. Consider using strncat_s instead.
:: /wd4505  unreferenced local function has been removed
set DISABLED_WARNINGS=/wd4100 /wd4189 /wd4996 /wd4505

:: Compiler flags
:: /std     use this version of c++
:: /Oi		compiler intrinsics
:: /GR- 	no runtime type info
:: /EHa- 	turn off exception handling
:: /nologo 	turn off compiler banner thing
:: /W4		warning level 4
:: /MT		statically link C runtime into executable for portability
:: /Gm-		Turn off incremental build
:: /Z7		Old style debugging info
:: /Fm		Generate map file
:: /Fe      Executable name
:: not set
:: /I       Additional include directory
:: /P       Preprocessor output to file
set COMMON_COMPILER_FLAGS=/std:c++17 /Oi /GR- /EHa- /nologo /W4 /MT /Gm- /Z7 /Fm %DISABLED_WARNINGS% /Fe%EXE_NAME% /I ..\game\include
set PLATFORM_COMPILER_FLAGS=/I %SDL_DIR%\include /I %STB_DIR% /I %IMGUI_DIR%\backends /I %IMGUI_DIR% /I %GLAD_DIR%\include
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %PLATFORM_COMPILER_FLAGS%

:: Linker flags
:: /opt:ref         remove unneeded stuff from .map file
:: /LIBPATH:        sdl library path, libraries to include, and additional arguments (enable console subsystem for debugging)
:: /INCREMENTAL:NO  perform a full link
:: /DEBUG           enable debugging
set PLATFORM_LINKER_FLAGS=/LIBPATH:%SDL_DIR%\lib\x64 SDL2.lib SDL2main.lib
set COMMON_LINKER_FLAGS=/OUT:%EXE_NAME% /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
set LINKER_FLAGS=%COMMON_LINKER_FLAGS% %PLATFORM_LINKER_FLAGS% /DEBUG:FULL

set IMGUI_SOURCES=%IMGUI_DIR%\backends\imgui_impl_sdl.cpp %IMGUI_DIR%\backends\imgui_impl_opengl3.cpp %IMGUI_DIR%\imgui*.cpp
set GAME_SOURCES=..\game\main.cpp ..\game\windows.c ..\game\log.c ..\game\mem.c ..\game\render.c ..\game\game.cpp ..\game\file.c
set SOURCES=%GAME_SOURCES% %IMGUI_SOURCES% %GLAD_DIR%\glad.c

:: Create build directory
IF NOT EXIST build mkdir build
pushd build

IF EXIST %EXE_NAME% del %EXE_NAME%

:: Preprocess only
:: cl /P %SOURCES% %COMPILER_FLAGS% %ADDITIONAL_FLAGS%

:: Build executable
cl %SOURCES% %COMPILER_FLAGS% %ADDITIONAL_FLAGS% /link %LINKER_FLAGS%

popd
copy build\%EXE_NAME% .

:: Get SDL runtime libraries
IF NOT EXIST SDL2.dll copy %SDL_DIR%\lib\x64\SDL2.dll .
