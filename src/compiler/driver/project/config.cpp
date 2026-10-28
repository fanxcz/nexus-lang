#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        fs::path findProjectRoot(fs::path start) {

            std::error_code ec;

            start = fs::weakly_canonical(start, ec);

            if (ec)
                start = fs::absolute(start, ec);

            if (fs::is_regular_file(start, ec))
                start = start.parent_path();

            for (;;) {

                if (fs::exists(start / "nexus.toml"))
                    return start;

                const auto parent = start.parent_path();

                if (parent == start)
                    break;

                start = parent;
            }
            return {};
        }


        ProjectConfig loadProject(const fs::path& requestedRoot ) {

            const fs::path root = findProjectRoot(requestedRoot);

            if (root.empty())
                throw std::runtime_error(
                    "nexus.toml not found; run `nexus init` or pass a .nx file");

            const std::string data = readFile(root / "nexus.toml");

            ProjectConfig cfg;

            cfg.root = root;

            cfg.name = tomlString(data, "package", "name");

            cfg.version = tomlString(data, "package", "version");

            cfg.entry = tomlString(data, "build", "entry");

            cfg.artifact = tomlString(data, "build", "artifact");

            cfg.target = tomlString(data, "build", "target");

            cfg.windowWidth = tomlInt(data, "window", "width", 1280);

            cfg.windowHeight = tomlInt(data, "window", "height", 720);

            cfg.windowTitle = tomlString(data, "window", "title");

            if (cfg.name.empty())
                cfg.name = root.filename().string();

            if (cfg.version.empty())
                cfg.version = kVersion;

            if (cfg.entry.empty())
                cfg.entry = "src/main.nx";

            if (cfg.artifact.empty())
                cfg.artifact = cfg.name;

            if (cfg.windowTitle.empty())
                cfg.windowTitle = cfg.name;

            return cfg;
        }

    } // namespace driver
} // namespace nexus
