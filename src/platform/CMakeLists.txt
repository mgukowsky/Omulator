# Select which platform folder to use
# N.B. "sdl" assumes the compiler is gcc or clang, NOT msvc...

if(MSVC)
  add_subdirectory("win32")
else()
  add_subdirectory("sdl")
endif()

