# Runtime, standard-library, templates and asset installation rules.

include(GNUInstallDirs)
# ============================================================================
target_compile_definitions(nexus PRIVATE NEXUS_INSTALL_RUNTIME_DIR="${CMAKE_INSTALL_FULL_DATADIR}/nexus/runtime")
install(TARGETS nexus RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# The runtime is one translation unit assembled from topic files, so the
# whole directory (runtime.cpp/runtime.c plus every *.inc part) must ship.
install(DIRECTORY src/runtime/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/runtime
        FILES_MATCHING PATTERN "*.cpp" PATTERN "*.c" PATTERN "*.inc" PATTERN "*.hpp")
install(DIRECTORY stdlib/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/stdlib OPTIONAL)
install(DIRECTORY examples/templates/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/project-templates OPTIONAL)
install(DIRECTORY examples/assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/assets OPTIONAL)
