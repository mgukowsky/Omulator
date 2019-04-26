# N.B. we need to have run find_package(SDL2) before we get here
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${SDL2_INCLUDE_DIR}
)

target_sources_local(
  ${PROJECT_NAME}
  PRIVATE
    PrimitiveIO.cpp  
)

