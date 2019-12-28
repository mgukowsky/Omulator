# Omulator
[![Build Status](https://travis-ci.com/mgukowsky/Omulator.svg?branch=master)](https://travis-ci.com/mgukowsky/Omulator)
[![Build Status](https://ci.appveyor.com/api/projects/status/lbl2dwjvl0is9tql/branch/master?svg=true)](https://ci.appveyor.com/project/mgukowsky/Omulator)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mgukowsky/Omulator/master/LICENSE)

The Omnibus Emulator

## Directory Structure
In an effort to make the code as portable as possible, all code outside the `platform` directory is designed to be as system-agnostic as possible (i.e. the Linux source tree's `arch` directory).
* `cmake` => files for the CMake build system
* `include` => header files for the project
    * N.B. we use the `.hpp` extension for headers that contain C++ code.
* `platform` => __platform-specific__ header and implementation files
    * `win32` => Windows with MSVC-specific
    * `sdl` => Linux with GCC or Clang
* `src` => implementation files for the project 
* `test` => source code for test programs (we use a separate executable for each unit/integration/e2e test) 
    * `unit` => test programs which verify the functionality of a single class/group of functions in __isolation__ (i.e. all dependencies are mocked where possible).
    * `integration` => test programs which verify the functionality of multiple classes/groups of functions when linked together. Dependencies are mocked where necessary.
    * `e2e` => end-to-end tests which verify the functionality of the fully-linked program.
* `third_party` => external dependencies, mostly tracked as git submodules.
