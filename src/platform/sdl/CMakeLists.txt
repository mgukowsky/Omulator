message(
  STATUS
    "Searching for SDL2; please ensure any relevant packages are installed (i.e. libsdl2-dev)"
)

# N.B. this uses our <project_root>/cmake/FindSDL2.cmake
find_package(SDL2 REQUIRED)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${SDL2_INCLUDE_DIR}
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
    ${SDL2_LIBRARY}
)

target_sources_local(
  ${PROJECT_NAME}
  PRIVATE
    PrimitiveIO.cpp  
)

