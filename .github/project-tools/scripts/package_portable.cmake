# Stage self-contained NEXUS distribution trees and archives.
#
# Usage:
#   cmake -DNEXUS_BIN=<path to built nexus binary>
#         -DSOURCE_DIR=<repository root>
#         -DOUT_DIR=<output directory, defaults to <root>/dist>
#         -P .github/project-tools/scripts/package_portable.cmake
#
# Layouts produced:
#   Windows: <OUT_DIR>/NEXUS_Windows_Portable/  (+ NEXUS_Windows_Portable.zip)
#     root/nexus.cmd, *.bat, *.ps1, bin/nexus-core.exe, stdlib/,
#     runtime/runtime.cpp, project-templates/, assets/, LICENSE, ...
#   Unix:    <OUT_DIR>/nexus-<ver>-<system>-x86_64/  (+ .tar.gz)
#     bin/nexus, share/nexus/{stdlib,runtime,project-templates,assets}, ...
#
# The builtin native backend needs nothing else. The LLVM backend still needs
# an external C++ compiler (clang++ on PATH, NEXUS_CXX, or a bundled
# Zig-backed clang++.cmd beside the binary); a pre-existing toolchain/
# directory next to the binary is picked up automatically but is NOT required
# for subset programs. Nothing is downloaded by this script.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
if(NOT DEFINED SOURCE_DIR)
  message(FATAL_ERROR "SOURCE_DIR is required")
endif()
if(NOT DEFINED OUT_DIR)
  set(OUT_DIR "${SOURCE_DIR}/dist")
endif()

file(READ "${SOURCE_DIR}/CMakeLists.txt" CMAKE_TXT)
string(REGEX MATCH "project\\(nexus VERSION ([0-9.]+)" _ "${CMAKE_TXT}")
set(NEXUS_VERSION "${CMAKE_MATCH_1}")
if(NOT NEXUS_VERSION)
  message(FATAL_ERROR "cannot determine version from CMakeLists.txt")
endif()

file(MAKE_DIRECTORY "${OUT_DIR}")

if(CMAKE_HOST_WIN32)
  set(STAGE "${OUT_DIR}/NEXUS_Windows_Portable")
  file(REMOVE_RECURSE "${STAGE}")
  file(MAKE_DIRECTORY "${STAGE}/bin")

  file(COPY "${NEXUS_BIN}" DESTINATION "${STAGE}/bin")
  get_filename_component(BIN_NAME "${NEXUS_BIN}" NAME)
  if(NOT BIN_NAME STREQUAL "nexus-core.exe")
    file(RENAME "${STAGE}/bin/${BIN_NAME}" "${STAGE}/bin/nexus-core.exe")
  endif()

  foreach(F nexus.cmd NEXUS-Windows.ps1 NEXUS-Console.bat Run-NEXUS.bat
      Build-NEXUS.bat Check-NEXUS-Toolchain.bat Install-NEXUS.bat
      Install-Requirements.bat Install-NEXUS-Path.ps1 Uninstall-NEXUS-Path.ps1
      Install-Windows-Association.ps1 Uninstall-Windows-Association.ps1
      README-WINDOWS-RU.txt LICENSE README.md THIRD_PARTY_NOTICES.md)
    set(SOURCE_FILE "${SOURCE_DIR}/${F}")
    if(NOT EXISTS "${SOURCE_FILE}")
      set(SOURCE_FILE "${SOURCE_DIR}/.github/project-tools/repository-files/${F}")
    endif()
    if(EXISTS "${SOURCE_FILE}")
      file(COPY "${SOURCE_FILE}" DESTINATION "${STAGE}")
    endif()
  endforeach()
  foreach(D stdlib)
    if(EXISTS "${SOURCE_DIR}/${D}")
      file(COPY "${SOURCE_DIR}/${D}" DESTINATION "${STAGE}")
    endif()
  endforeach()
  if(EXISTS "${SOURCE_DIR}/examples/templates")
    file(COPY "${SOURCE_DIR}/examples/templates" DESTINATION "${STAGE}")
    file(RENAME "${STAGE}/templates" "${STAGE}/project-templates")
  endif()
  if(EXISTS "${SOURCE_DIR}/examples/assets")
    file(COPY "${SOURCE_DIR}/examples/assets" DESTINATION "${STAGE}")
  endif()
  file(MAKE_DIRECTORY "${STAGE}/runtime")
  foreach(R src/runtime/runtime.cpp src/runtime/runtime.c)
    if(EXISTS "${SOURCE_DIR}/${R}")
      file(COPY "${SOURCE_DIR}/${R}" DESTINATION "${STAGE}/runtime")
    endif()
  endforeach()
  # An operator-provided toolchain/ directory (Zig-backed clang++.cmd) is used
  # when present but is optional since the builtin native backend arrived.
  if(EXISTS "${SOURCE_DIR}/toolchain")
    file(COPY "${SOURCE_DIR}/toolchain" DESTINATION "${STAGE}")
  endif()
  file(WRITE "${STAGE}/VERSION.txt" "${NEXUS_VERSION}\n")

  set(ARCHIVE "${OUT_DIR}/NEXUS_Windows_Portable.zip")
  file(REMOVE "${ARCHIVE}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${ARCHIVE}" --format=zip NEXUS_Windows_Portable
    WORKING_DIRECTORY "${OUT_DIR}"
    RESULT_VARIABLE rc
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "cannot create ${ARCHIVE}")
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E sha256sum NEXUS_Windows_Portable.zip
    WORKING_DIRECTORY "${OUT_DIR}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE sum
  )
  if(rc EQUAL 0)
    string(REGEX MATCH "[0-9a-f]+" SHORT "${sum}")
    file(WRITE "${ARCHIVE}.sha256.txt" "${SHORT}  NEXUS_Windows_Portable.zip\n")
  endif()
  message(STATUS "Windows portable package: ${ARCHIVE}")
else()
  string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" SYS)
  set(STAGE "${OUT_DIR}/nexus-${NEXUS_VERSION}-${SYS}-x86_64")
  file(REMOVE_RECURSE "${STAGE}")
  file(MAKE_DIRECTORY "${STAGE}/bin" "${STAGE}/share/nexus")

  file(COPY "${NEXUS_BIN}" DESTINATION "${STAGE}/bin")
  get_filename_component(BIN_NAME "${NEXUS_BIN}" NAME)
  if(NOT BIN_NAME STREQUAL "nexus")
    file(RENAME "${STAGE}/bin/${BIN_NAME}" "${STAGE}/bin/nexus")
  endif()
  execute_process(COMMAND chmod 755 "${STAGE}/bin/nexus")

  foreach(D stdlib)
    if(EXISTS "${SOURCE_DIR}/${D}")
      file(COPY "${SOURCE_DIR}/${D}" DESTINATION "${STAGE}/share/nexus")
    endif()
  endforeach()
  if(EXISTS "${SOURCE_DIR}/examples/templates")
    file(COPY "${SOURCE_DIR}/examples/templates" DESTINATION "${STAGE}/share/nexus")
    file(RENAME "${STAGE}/share/nexus/templates" "${STAGE}/share/nexus/project-templates")
  endif()
  if(EXISTS "${SOURCE_DIR}/examples/assets")
    file(COPY "${SOURCE_DIR}/examples/assets" DESTINATION "${STAGE}/share/nexus")
  endif()
  file(MAKE_DIRECTORY "${STAGE}/share/nexus/runtime")
  foreach(R src/runtime/runtime.cpp src/runtime/runtime.c)
    if(EXISTS "${SOURCE_DIR}/${R}")
      file(COPY "${SOURCE_DIR}/${R}" DESTINATION "${STAGE}/share/nexus/runtime")
    endif()
  endforeach()
  foreach(F LICENSE README.md THIRD_PARTY_NOTICES.md)
    set(SOURCE_FILE "${SOURCE_DIR}/${F}")
    if(NOT EXISTS "${SOURCE_FILE}")
      set(SOURCE_FILE "${SOURCE_DIR}/.github/project-tools/repository-files/${F}")
    endif()
    if(EXISTS "${SOURCE_FILE}")
      file(COPY "${SOURCE_FILE}" DESTINATION "${STAGE}")
    endif()
  endforeach()
  if(EXISTS "${SOURCE_DIR}/.github/project-tools/installer/install.sh")
    file(COPY "${SOURCE_DIR}/.github/project-tools/installer/install.sh" DESTINATION "${STAGE}")
    execute_process(COMMAND chmod 755 "${STAGE}/install.sh")
  endif()
  file(WRITE "${STAGE}/VERSION.txt" "${NEXUS_VERSION}\n")

  get_filename_component(STAGE_NAME "${STAGE}" NAME)
  set(ARCHIVE "${OUT_DIR}/${STAGE_NAME}.tar.gz")
  file(REMOVE "${ARCHIVE}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${ARCHIVE}" --format=gnutar "${STAGE_NAME}"
    WORKING_DIRECTORY "${OUT_DIR}"
    RESULT_VARIABLE rc
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "cannot create ${ARCHIVE}")
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E sha256sum "${STAGE_NAME}.tar.gz"
    WORKING_DIRECTORY "${OUT_DIR}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE sum
  )
  if(rc EQUAL 0)
    string(REGEX MATCH "[0-9a-f]+" SHORT "${sum}")
    file(WRITE "${ARCHIVE}.sha256.txt" "${SHORT}  ${STAGE_NAME}.tar.gz\n")
  endif()
  message(STATUS "Unix portable package: ${ARCHIVE}")
endif()
