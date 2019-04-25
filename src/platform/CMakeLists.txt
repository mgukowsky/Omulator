# Select which platform folder to use

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_subdirectory("msvc")
else()
  add_subdirectory("sdl")
endif()

