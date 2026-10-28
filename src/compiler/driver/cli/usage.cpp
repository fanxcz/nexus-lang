#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        void usage() {

            std::cout
                << "Nexus " << kVersion << "\n"
                << "Usage: nexus <command> [options]\n\n"
                << "Interactive shell:\n"
                << "  repl                 evaluate expressions (also starts by running nexus with "
                   "no arguments)\n\n"
                << "Project commands:\n"
                << "  new <project> [--template cli|app|game|3d|paint|server|library]\n"
                << "  init [--template cli|app|game|3d|paint|server|library]\n"
                << "  info\n"
                << "  assets\n"
                << "  clean\n"
                << "  doctor [--json]\n"
                << "  self-test\n"
                << "  explain-backend <file.nx> [--target TRIPLE]\n"
                << "  editor [--gui]\n\n"
                << "Build commands:\n"
                << "  build [file.nx] [-o output] [--target TRIPLE] [--backend auto|native|llvm] "
                   "[--emit-ir] [--release]\n"
                << "  run [file.nx] [--target TRIPLE] [--backend auto|native|llvm] [--release]\n"
                << "  export [--target TRIPLE] [--backend auto|native|llvm] [-o output]\n"
                << "  check [file.nx]\n"
                << "  targets\n"
                << "  fmt [file.nx] [--check]\n"
                << "  lint [file.nx]\n"
                << "  doc\n"
                << "  bench [file.nx] [--runs N]\n"
                << "  test\n"
                << "  pkg <init|add|remove|list|update|build|publish> [args]\n"
                << "  lsp\n"
                << "  version\n"
                << "  help\n\n"
                << "Backends:\n"
                << "  auto     use the builtin native backend when the program fits\n"
                << "           its subset, otherwise the LLVM backend (default)\n"
                << "  native   builtin direct machine-code backend, no external\n"
                << "           compiler or linker (x86-64 Windows/Linux subset)\n"
                << "  llvm     LLVM IR + external C++ compiler (full language)\n\n"
                << "Examples:\n"
                << "  nexus new NeonGame --template game\n"
                << "  cd NeonGame && nexus build && nexus run\n"
                << "  nexus check src/main.nx\n"
                << "  nexus build src/main.nx -o app --target x86_64-linux\n";
        }

    } // namespace driver
} // namespace nexus
