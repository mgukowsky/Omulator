# Utilities to define the target platform and compiler
function(define_target_arch target_name)
  # if x64, will be AMD64 on Windows or x86_64 on Linux
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    target_compile_definitions(
      ${target_name}
      PUBLIC
        OML_ARCH_X64
    )
  # should be "arm" on ARM Linux (I think?)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    target_compile_definitions(
      ${target_name}
      PUBLIC
        OML_ARCH_ARM
    )
  else()
    message(FATAL_ERROR "Unknown architecture...")
  endif()
endfunction()

function(define_target_compiler target_name)
  if(MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(
      ${target_name}
      PUBLIC
        OML_COMPILER_CLANG_CL
    )
  elseif(MSVC)
    target_compile_definitions(
      ${target_name}
      PUBLIC
        OML_COMPILER_MSVC
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_definitions(
      ${target_name}
      PUBLIC
        OML_COMPILER_GCC
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(
      ${target_name}
      PUBLIC
        OML_COMPILER_CLANG
    )
  else()
    message(FATAL_ERROR "Unknown compiler: " ${CMAKE_CXX_COMPILER_ID})
  endif()
endfunction()
