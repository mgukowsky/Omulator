# CMake likes to insert a bunch of garbage for MSVC, and interferes with flags on other platforms
set(CMAKE_CXX_FLAGS_INIT "")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "")
#set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "")

set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")
#set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT "")

set(CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT "")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "")
#set(CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL_INIT "")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "")
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT "")

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT "")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "")
#set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL_INIT "")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "")
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT "")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "")
