#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        int commandInfo() {

            const auto cfg = loadProject();

            std::cout << "NEXUS project\n"
                      << "  name:    " << cfg.name << "\n"
                      << "  version: " << cfg.version << "\n"
                      << "  root:    " << cfg.root << "\n"
                      << "  entry:   " << cfg.entry << "\n"
                      << "  artifact:" << cfg.artifact << "\n"
                      << "  target:  " << cfg.target << "\n"
                      << "  window:  " << cfg.windowWidth << "x" << cfg.windowHeight << "\n"
                      << "  title:   " << cfg.windowTitle << "\n";

            return 0;
        }


        int commandAssets() {

            const auto cfg = loadProject();

            const std::vector<fs::path> dirs = {

                cfg.root / "assets/textures",
                cfg.root / "assets/audio",
                cfg.root / "assets/models",
                cfg.root / "assets/fonts",
                cfg.root / "scenes"};

            std::size_t count = 0;

            std::cout << "Assets for " << cfg.name << ":\n";

            for (const auto& dir : dirs) {

                if (!fs::exists(dir))
                    continue;

                for (const auto& e : fs::recursive_directory_iterator(dir)) {

                    if (!e.is_regular_file())
                        continue;

                    ++count;

                    std::cout << "  " << fs::relative(e.path(), cfg.root).string() << "\n";
                }
            }
            std::cout << "Total: " << count << " file(s)\n";

            return 0;
        }


        int commandClean() {

            const auto cfg = loadProject();

            std::error_code ec;

            fs::remove_all(cfg.root / "build", ec);

            if (ec)
                throw std::runtime_error("cannot remove build directory: " + ec.message());

            std::cout << "clean: OK\n";

            return 0;
        }

    } // namespace driver
} // namespace nexus
