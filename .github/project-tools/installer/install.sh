#!/bin/sh
# NEXUS installer for Linux and macOS (self-contained package).
#
# Install:  ./install.sh [--prefix DIR] [--uninstall]
# Default prefix: $HOME/.local/share/nexus
# A `nexus` launcher symlink is placed in $HOME/.local/bin (created if needed).
# Add that directory to PATH once (the script prints the exact line if missing).
#
# Requires nothing except /bin/sh, mkdir, cp, rm, ln. No compiler toolchain,
# SDK, SDL, CMake, or Ninja is needed: subset programs compile with the
# builtin native backend. Full-language programs additionally need a C++
# compiler for the LLVM backend (clang++ on PATH or NEXUS_CXX); see
# `nexus doctor`.
set -eu

PREFIX="${HOME}/.local/share/nexus"
ACTION="install"

for arg in "$@"; do
  case "$arg" in
    --prefix=*) PREFIX="${arg#--prefix=}";;
    --prefix) echo "usage: $0 [--prefix DIR] [--uninstall]" >&2; exit 2;;
    --uninstall) ACTION="uninstall";;
    -h|--help) echo "usage: $0 [--prefix DIR] [--uninstall]"; exit 0;;
    *) echo "unknown option: $arg" >&2; exit 2;;
  esac
done

BIN_DIR="${HOME}/.local/bin"

if [ "$ACTION" = "uninstall" ]; then
  rm -f "${BIN_DIR}/nexus"
  rm -rf "${PREFIX}"
  echo "NEXUS uninstalled (prefix ${PREFIX}, launcher ${BIN_DIR}/nexus)."
  exit 0
fi

# The script lives either inside the extracted package (bin/ next to it) or
# beside the tarball; locate the staged bin/ and share/ directories.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -x "${HERE}/bin/nexus" ]; then
  STAGE="$HERE"
elif [ -x "${HERE}/nexus-1.1.0-linux-x86_64/bin/nexus" ]; then
  STAGE="${HERE}/nexus-1.1.0-linux-x86_64"
else
  # Fall back to globbing any extracted nexus-*-x86_64 tree.
  STAGE=""
  for d in "${HERE}"/nexus-*-x86_64; do
    if [ -x "$d/bin/nexus" ]; then STAGE="$d"; break; fi
  done
  if [ -z "$STAGE" ]; then
    echo "error: cannot find an extracted NEXUS package next to install.sh" >&2
    echo "extract the .tar.gz first, then run <package>/install.sh" >&2
    exit 1
  fi
fi

mkdir -p "${PREFIX}" "${BIN_DIR}"
cp -R "${STAGE}/bin" "${STAGE}/share" "${PREFIX}/"
if [ -f "${STAGE}/LICENSE" ]; then cp "${STAGE}/LICENSE" "${PREFIX}/"; fi
if [ -f "${STAGE}/THIRD_PARTY_NOTICES.md" ]; then cp "${STAGE}/THIRD_PARTY_NOTICES.md" "${PREFIX}/"; fi
if [ -f "${STAGE}/VERSION.txt" ]; then cp "${STAGE}/VERSION.txt" "${PREFIX}/"; fi
chmod 755 "${PREFIX}/bin/nexus"
ln -sf "${PREFIX}/bin/nexus" "${BIN_DIR}/nexus"

echo "NEXUS installed to ${PREFIX}"
echo "Launcher: ${BIN_DIR}/nexus"

case ":${PATH}:" in
  *":${BIN_DIR}:"*) ;;
  *)
    echo "Add ${BIN_DIR} to PATH once, e.g.:"
    echo "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.profile"
    ;;
esac

# Self-check: version, diagnostics, and a toolchain-free native build.
"${BIN_DIR}/nexus" --version
"${BIN_DIR}/nexus" doctor || true
CHECK_DIR="$(mktemp -d)"
trap 'rm -rf "$CHECK_DIR"' EXIT INT TERM
printf 'fn main() { print("NEXUS_INSTALL_OK") }\n' > "${CHECK_DIR}/hello.nx"
(cd "$CHECK_DIR" && env -u NEXUS_CXX "${BIN_DIR}/nexus" run hello.nx --backend native)
echo "Install check: PASS (native build works with no NEXUS_CXX)"
echo "Uninstall: install.sh --prefix ${PREFIX} --uninstall"
