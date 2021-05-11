# Toolchain that allows MSVC to be used within WSL. Assumes that all of these .exe files can
# be found in $PATH. The easiest way to do this is to run the 'vcvars64.bat' script that ships with
# Visual Studio in a shell/shortcut prior to launching wsl.exe in order to set all of the required
# variables.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR "AMD64")

set(CMAKE_C_COMPILER cl.exe)
set(CMAKE_CXX_COMPILER cl.exe)
set(CMAKE_LINKER link.exe)
set(CMAKE_AR lib.exe)
set(CMAKE_MT mt.exe)
set(CMAKE_RC rc.exe)
