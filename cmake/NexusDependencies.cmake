# Optional runtime dependencies shared by compiler and runtime-linked tests.
# Keep these lookups after project() so platform/toolchain detection is initialized.

# ============================================================================
# SDL2 - try vcpkg first, then pkg-config, then system paths
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(SDL2 QUIET sdl2)
  pkg_check_modules(SDL2_IMAGE QUIET SDL2_image)
  pkg_check_modules(SDL2_MIXER QUIET SDL2_mixer)
  pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
endif()

# If not found via pkg-config, try find_package (works with vcpkg)
if (NOT SDL2_FOUND)
  find_package(SDL2 QUIET)
  find_package(SDL2_image QUIET)
  find_package(SDL2_mixer QUIET)
  find_package(SDL2_ttf QUIET)
endif()

if (NOT SDL2_FOUND)
  message(WARNING "SDL2 not found! Graphics runtime will not work.")
  message(WARNING "Install SDL2 development libraries:")
  message(WARNING "  Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev")
  message(WARNING "  Arch: sudo pacman -S sdl2 sdl2_image sdl2_mixer sdl2_ttf")
  message(WARNING "  Fedora: sudo dnf install SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel")
  message(WARNING "  Windows (vcpkg): vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf --triplet x64-windows")
  message(WARNING "  Windows (choco): choco install sdl2")
  message(WARNING "  macOS: brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf")
endif()
