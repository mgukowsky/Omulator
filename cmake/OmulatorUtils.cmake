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

function(configure_target target_name)
  define_target_arch(${target_name})
  define_target_compiler(${target_name})

  target_compile_features(
    ${target_name}
    PUBLIC
      cxx_std_17
  )

  set_target_properties(
    ${target_name}
    PROPERTIES
      CXX_STANDARD
        17
      CXX_STANDARD_REQUIRED
        ON
      CXX_EXTENSIONS
        OFF
      POSITION_INDEPENDENT_CODE
        ON
  )

  target_compile_definitions(
    ${target_name}
    PUBLIC
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:NDEBUG>

      # _REENTRANT signals parts of the stdlib to use threadsafe functions
      $<$<BOOL:UNIX>:_REENTRANT>
  )
  
  if(MSVC)
    config_for_msvc(${target_name})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    config_for_gcc(${target_name})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    config_for_clang(${target_name})
  endif()

endfunction()

function(config_for_msvc target_name)

  target_compile_options(
    ${target_name}
    PUBLIC
      # Allow generation of AVX instructions
      /arch:AVX2

      # RTTI
      /GR

      # ISO C++ Exceptions
      /EHsc

      # Language options
      /fp:precise
      /permissive-
      /volatile:iso

      # ISO compliancy
      /Zc:forScope
      /Zc:inline
      /Zc:rvalueCast
      /Zc:strictStrings
      /Zc:ternary
      /Zc:wchar_t

      # Warnings and additional output
      /diagnostics:classic
      /Fa${CMAKE_CURRENT_BINARY_DIR}/
      /WX

      # Additional optimizations for x64, off for now...
      #/favor:INTEL64

      # /Wall in MSVC is a bit overzealous, so we disable some warnings we don't care about
      # 4307: integral constant overflow
      # 4514: compiler removed an unreferenced inline function
      # 4571: catch(...) won't work with SEH
      # 4625: copy constructor implicity deleted; allows us to use rule of 0/3/5
      # 4626: assignment operator implicitly deleted; allows us to use rule of 0/3/5
      # 4668: macro was undefined, so compiler replaced with '0'
      # 4710: function could not be inlined
      # 4711: function was automatically inlined
      # 4820: compiler needed to insert padding in class/struct
      # 4946: reinterpret_cast between related classes (i.e. reinterpret_cast<Derived*>(Base*))
      # 5045: Spectre mitigation with /Qspectre (range checks)
      /wd4307
      /wd4514
      /wd4571
      /wd4625
      /wd4626
      /wd4668
      /wd4710
      /wd4711
      /wd4820
      /wd4946
      /wd5045

      # Experimental MSVC feature which ignores errors from system headers. N.B. this
      # will eventually no longer be experimental and will have to change. This sets
      # warnings from <angle bracket> headers to W0. Needs to be off for clang-cl, though.
      $<$<CXX_COMPILER_ID:MSVC>:/experimental:external /external:anglebrackets /external:W0>

      # Create an assembly dump, force ISO compliancy
      $<$<CXX_COMPILER_ID:MSVC>:/FAcs /Zc:referenceBinding /Zc:throwingNew>

      # MSVC's Wall is ridiculous and triggers a deluge of false positives in its OWN headers,
      # so W4 is the next best thing. WL puts diagnostics on one line.
      $<$<CXX_COMPILER_ID:MSVC>:/W4 /WL>

      # Stop clang-cl from complaining about Windows code
      $<$<CXX_COMPILER_ID:Clang>:-W3 -Wno-c++98-compat -Wno-c++98-compat-pedantic>

      # Make a PDB, don't inline and don't optimize, use buffer overflow canaries, and add 
      # security and runtime checks
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:/Zi /Ob0 /Od /GS /sdl /RTC1>

      # Maximum optimization, optimize globals, no buffer overflow canaries, and 
      # function level linking
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:/O2 /Ob2 /Gw /GS- /Gy>
  )

  target_compile_definitions(
    ${target_name}
    PUBLIC
      _MBCS
      _WINDOWS
      WIN32
      # Magic macro to limit how much of the Windows header gets pulled in
      WIN32_LEAN_AND_MEAN
  )

  # We want fine-grained control over the MSVC linker...
  target_link_options(
    ${target_name}
    PUBLIC
      # ASLR
      /DYNAMICBASE
      # 64-bit ASLR
      /HIGHENTROPYVA
      # don't use incremental linking
      /INCREMENTAL:NO
      # 64-bit address space
      /LARGEADDRESSAWARE
      # Use NX bit (aka Windows Data Execution Prevention)
      /NXCOMPAT
      # 64 bit
      /machine:x64
      # an additional object to link against which prevents MSVC telemetry
      notelemetry.obj

      # /DEBUG:FASTLINK is (obviously) faster for the linker, but /DEBUG:FULL
      # seems to play better with GoogleTest in debug builds
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:/DEBUG:FULL>

      # LTCG: link time code generation; lld-link seems to not understand neither /LTCG or -flto ¯\_(ツ)_/¯
      # OPT:REF: remove unreferenced functions and data
      # OPT:ICF: merge identical functions and data
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:$<$<CXX_COMPILER_ID:MSVC>:/LTCG> /OPT:REF /OPT:ICF>
  )

  if(CMAKE_BUILD_TYPE MATCHES Release AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(
      ${target_name}
      PUBLIC
        # Whole program optimization
        /GL

        # Fast transcendentals
        /Qfast_transcendentals
    )
  endif()

  # For Windows targets, manifest files can be added as source files and will be embedded
  # in the resulting binary (https://cmake.org/cmake/help/v3.4/release/3.4.html#other)
  #
  # This particular manifest is needed for proper DPI handling
  #
  # N.B. the path we're using for the DPI file assumes that we are using an MSVC build environment
  set(PerMonitorDPIAwareFile "$ENV{VCToolsInstallDir}INCLUDE\\MANIFEST\\PERMONITORHIGHDPIAWARE.MANIFEST")
  if(EXISTS ${PerMonitorDPIAwareFile})
    target_sources(
      ${target_name}
      PUBLIC
         ${PerMonitorDPIAwareFile}
    )
  else()
    message(
      WARNING
      "warning: could not file the DPI manifest file \"${PerMonitorDPIAwareFile}\""
    )
  endif()

endfunction()

function(config_for_gcc target_name)
  target_compile_options(
    ${target_name}
    PUBLIC
      -ansi
      -pedantic
      -Wall
      -Werror
      -Wextra
      -Wshadow
      -Wwrite-strings

      # Target Intel Broadwell (~2015 w/ AVX2)
      # on an official build machine this could be -march=native
      -march=broadwell

      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:-g -O0>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:-O3 -fomit-frame-pointer>
  )

  target_link_options(
    ${target_name}
    PUBLIC
      # -rdynamic can help play nicely with backtraces
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:-g -rdynamic>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:-Wl,-O3>

      # --relax enables global addressing optimizations if using GCC
      $<$<CXX_COMPILER_ID:GNU>:-Wl,--relax>
  )
endfunction()

function(config_for_clang target_name)
  config_for_gcc(${target_name})

  target_compile_options(
    ${target_name}
    PUBLIC
      # -fno-omit-frame-pointer is needed to help the sanitizers
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},RelWithDebInfo>:-g -O1 -fno-omit-frame-pointer>

      # Clang sanitizers (ASAN and UBSAN)
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},RelWithDebInfo>:-fsanitize=address -fsanitize=undefined>

      # Additional sanitizers which are incompatible with ASAN, but should be added to a
      # separate build...
      # $<$<STREQUAL:${CMAKE_BUILD_TYPE},RelWithDebInfo>: -fsanitize=memory>
      # $<$<STREQUAL:${CMAKE_BUILD_TYPE},RelWithDebInfo>: -fsanitize=thread>
  )

  target_link_options(
    ${target_name}
    PUBLIC
      # Need to reclare sanitizers for the linker
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},RelWithDebInfo>:-g -fsanitize=address -fsanitize=undefined>

      # Use the LLVM linker if sanitizers aren't needed
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:-fuse-ld=lld>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:-fuse-ld=lld>
  )
endfunction()
