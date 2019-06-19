# Select which platform folder to use

if(MSVC)
  add_subdirectory("win32")
else()
  add_subdirectory("sdl")
endif()

