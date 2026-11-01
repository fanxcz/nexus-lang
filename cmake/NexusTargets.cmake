# Compiler executable target and its platform/runtime link configuration.
# Source grouping is provided by NexusSources.cmake.

add_executable(nexus ${NEXUS_SOURCES})
# The official Windows portable package must run without an installed
# Visual C++ redistributable. Statically link the MSVC CRT into the compiler.
if (WIN32)
  # Applies to MSVC-ABI compilers (including compatible Clang builds).
  # Harmless for toolchains that use a different runtime model.
  set_property(TARGET nexus PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_include_directories(nexus PRIVATE
    src/compiler
    src/compiler/frontend
    src/compiler/backend)
target_compile_features(nexus PRIVATE cxx_std_20)
target_compile_definitions(nexus PRIVATE NEXUS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}" NEXUS_VERSION="${PROJECT_VERSION}")
target_compile_options(nexus PRIVATE ${NEXUS_COMPILE_OPTIONS})
if (NEXUS_LINK_FLAG)
  target_link_options(nexus PRIVATE ${NEXUS_LINK_FLAG})
endif()

# Link SDL2 if found
if (SDL2_FOUND)
  target_link_libraries(nexus PRIVATE ${SDL2_LIBRARIES})
  target_include_directories(nexus PRIVATE ${SDL2_INCLUDE_DIRS})
  if (SDL2_IMAGE_FOUND)
    target_link_libraries(nexus PRIVATE ${SDL2_IMAGE_LIBRARIES})
  endif()
  if (SDL2_MIXER_FOUND)
    target_link_libraries(nexus PRIVATE ${SDL2_MIXER_LIBRARIES})
  endif()
  if (SDL2_TTF_FOUND)
    target_link_libraries(nexus PRIVATE ${SDL2_TTF_LIBRARIES})
  endif()
endif()
