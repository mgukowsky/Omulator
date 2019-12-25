function(add_clang_tidy_pass target_name)
  find_program(
    CLANG_TIDY_EXE
    NAMES
      "clang-tidy"
    DOC
      "Path to clang-tidy"
  )

  if(CLANG_TIDY_EXE)
    set(
      DO_CLANG_TIDY
      "${CLANG_TIDY_EXE}"
    )

    set_target_properties(
      ${target_name}
      PROPERTIES
        CXX_CLANG_TIDY
          ${DO_CLANG_TIDY}
    )
  endif()
endfunction()

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

function(configure_target target_name is_test)
  target_compile_features(
    ${target_name}
    PUBLIC
      cxx_std_17
  )

  set_target_properties(
    ${target_name}
    PROPERTIES
      CXX_EXTENSIONS
        OFF
      POSITION_INDEPENDENT_CODE
        ON
  )

  define_target_arch(${target_name})
  define_target_compiler(${target_name})

  target_compile_definitions(
    ${target_name}
    PUBLIC
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:NDEBUG>

      # _REENTRANT signals parts of the stdlib to use threadsafe functions
      $<$<BOOL:UNIX>:_REENTRANT>
  )
  
  if(MSVC)
    config_for_msvc(${target_name} ${is_test})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    config_for_gcc(${target_name} ${is_test})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    config_for_clang(${target_name} ${is_test})
  endif()

endfunction()

# For whatever reason, the conventional gtest_force_shared_crt flag
# DOES NOT WORK with my CMake setup. Given this, we pass in the is_test
# flag to determine which runtime we should link against (especially
# important for MSVC)
function(config_for_msvc target_name is_test)

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
  )

  # While most flags will work with MSVC and clang-cl, certain flags are specific to
  # each compiler
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(
      ${target_name}
      PUBLIC
        # Experimental MSVC feature which ignores errors from system headers. N.B. this
        # will eventually no longer be experimental and will have to change. This sets
        # warnings from <angle bracket> headers to W0. Needs to be off for clang-cl, though.
        /experimental:external
        /external:anglebrackets
        /external:W0

        # Create assembly dump
        /FAcs

        # ISO compliancy
        /Zc:referenceBinding
        /Zc:throwingNew

        # All warnings
        /Wall

        # One line diagnostics
        /WL
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
      ${target_name}
      PUBLIC
        # clang-cl barfs on a bunch of headers with higher warn levels (but not on Linux?)
        -W3
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
    )
  endif()


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
  target_compile_options(
    ${target_name}
    PUBLIC
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
  set_target_properties(
    ${target_name}
    PROPERTIES
      LINK_FLAGS
        # DYNAMICBASE: ASLR
        # HIGHENTROPYVA: 64-bit ASLR
        # INCREMENTAL:NO: don't use incremental linking
        # LARGEADDRESSAWARE: 64-bit address space
        # NXCOMPAT: Use NX bit (aka Windows Data Execution Prevention)
        # machine:x64: 64 bit
        # notelemetry.obj: an additional object to link against which prevents MSVC telemetry
        "/DYNAMICBASE /HIGHENTROPYVA /INCREMENTAL:NO /LARGEADDRESSAWARE /NXCOMPAT /machine:x64 notelemetry.obj"
      LINK_FLAGS_DEBUG
        # /DEBUG:FASTLINK is (obviously) faster for the linker, but /DEBUG:FULL
        # seems to play better with GoogleTest in debug builds
        "/DEBUG:FULL /INCREMENTAL:NO"
      LINK_FLAGS_RELEASE
        # LTCG: link time code generation
        # OPT:REF: remove unreferenced functions and data
        # OPT:ICF: merge identical functions and data
        "/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO"
  )

  # Release vs Debug stuff
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(
      ${target_name}
      PUBLIC
        # Make a PDB
        /Zi

        # Don't inline and don't optimize
        /Ob0
        /Od

        # Buffer overflow canaries
        /GS 

        # Security & Runtime checks
        /sdl
        /RTC1
    )

    # See comment at the top of this function for why this is necessary
    if(NOT ${is_test})
      target_compile_options(
        ${target_name}
        PUBLIC
          # Multithreaded debug shared runtime
          /MDd
      )
    endif()

  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(
      ${target_name}
      PUBLIC
        # Maximum optimization
        /O2 /Ob2

        # Optimize globals
        /Gw

        # No buffer overflow canaries
        /GS-

        # Function level linking
        /Gy
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(
        ${target_name}
        PUBLIC
          # Whole program optimization
          /GL

          # Fast transcendentals
          /Qfast_transcendentals
      )
    endif()

    # See comment at the top of this function for why this is necessary
    if(NOT ${is_test})
      target_compile_options(
        ${target_name}
        PUBLIC
          # Multithreaded shared runtime
          /MD
      )
    endif()

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

function(config_for_gcc target_name is_test)
  target_compile_options(
    ${target_name}
    PUBLIC
      -ansi
      -pedantic
      -Wall
      -Werror
      -Wextra
      #TODO: For some reason, -Wshadow causes errors in spdlog on gcc, but not clang...
      -Wshadow
      -Wwrite-strings

      # Target Intel Broadwell (~2015 w/ AVX2)
      # on an official build machine this could be -march=native
      -march=broadwell
  )

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(
      ${target_name}
      PUBLIC
        -g
        -O0
        -rdynamic
    )
  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(
      ${target_name}
      PUBLIC
        -O3
        -fomit-frame-pointer
        -flto
        -Wl,-O3

    )
  endif()
endfunction()

function(config_for_clang target_name is_test)
  target_compile_options(
    ${target_name}
    PUBLIC
      -ansi
      -pedantic
      -Wall
      -Werror
      -Wextra
      -Wshadow

      # Target Intel Broadwell (~2015 w/ AVX2)
      # on an official build machine this could be march-=native
      -march=broadwell
  )

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(
      ${target_name}
      PUBLIC
        -g
        -O0
    )
  elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    target_compile_options(
      ${target_name}
      PUBLIC
        -g
        -O1
        -fsanitize=address
        
        # Incompatible with ASAN, but should be added to a separate build...
        # Don't forget to add them to the linker flags when added back in!!!
        #-fsanitize=memory
        #-fsanitize=thread
        
        -fsanitize=undefined
        -fno-omit-frame-pointer

        # RelWithDebInfo uses sanitizers
    )

    set_target_properties(
      ${target_name}
      PROPERTIES
        LINK_FLAGS_RELWITHDEBINFO
          "-fsanitize=address -fsanitize=undefined"
    )

  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(
      ${target_name}
      PUBLIC
        -O3
        -fomit-frame-pointer
    )
  #  set_target_properties(
  #      ${target_name}
  #     PROPERTIES
  #       LINK_FLAGS_RELEASE
  #         "-flto"
  #   )
  endif()
endfunction()
